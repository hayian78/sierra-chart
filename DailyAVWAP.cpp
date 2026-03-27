#include "sierrachart.h"
SCDLLName("DailyAVWAP")
static const char* VERSION = "1.3.0";

SCSFExport scsf_DailyAVWAP(SCStudyInterfaceRef sc)
{
	SCSubgraphRef Subgraph_VWAP = sc.Subgraph[0];
	SCSubgraphRef Subgraph_StdDev1Pos = sc.Subgraph[1];
	SCSubgraphRef Subgraph_StdDev1Neg = sc.Subgraph[2];
	SCSubgraphRef Subgraph_StdDev2Pos = sc.Subgraph[3];
	SCSubgraphRef Subgraph_StdDev2Neg = sc.Subgraph[4];
	SCSubgraphRef Subgraph_StdDev3Pos = sc.Subgraph[5];
	SCSubgraphRef Subgraph_StdDev3Neg = sc.Subgraph[6];

	SCInputRef Input_ResetTime = sc.Input[0];
	SCInputRef Input_AlertSound = sc.Input[1];
	SCInputRef Input_AlertMessage = sc.Input[2];
	SCInputRef Input_StdDev1Mult = sc.Input[3];
	SCInputRef Input_StdDev2Mult = sc.Input[4];
	SCInputRef Input_StdDev3Mult = sc.Input[5];

	if (sc.SetDefaults)
	{
		sc.GraphName = "Daily AVWAP";
		sc.GraphShortName = "DAVWAP";
		sc.StudyDescription = "Daily AVWAP v1.2.0 - Calculates VWAP anchored daily at a configurable reset time (e.g. 17:00 for CME session boundary). Uses Last/Close price with configurable standard deviation bands. Automatically resets when the anchor time is crossed.";

		sc.GraphRegion = 0; // Overlay on main chart
		sc.AutoLoop = 0; // Manual loop for running-total accumulation pattern (requires sequential access)
		sc.ValueFormat = VALUEFORMAT_INHERITED;

		Subgraph_VWAP.Name = "VWAP";
		Subgraph_VWAP.DrawStyle = DRAWSTYLE_LINE;
		Subgraph_VWAP.LineWidth = 2;
		Subgraph_VWAP.PrimaryColor = RGB(255, 0, 255); // Magenta
		Subgraph_VWAP.DrawZeros = false;
		
		Subgraph_StdDev1Pos.Name = "+0.5 StdDev";
		Subgraph_StdDev1Pos.DrawStyle = DRAWSTYLE_IGNORE; 
		Subgraph_StdDev1Pos.PrimaryColor = RGB(0, 255, 0);
		
		Subgraph_StdDev1Neg.Name = "-0.5 StdDev";
		Subgraph_StdDev1Neg.DrawStyle = DRAWSTYLE_IGNORE; 
		Subgraph_StdDev1Neg.PrimaryColor = RGB(255, 0, 0);

		Subgraph_StdDev2Pos.Name = "+1.0 StdDev";
		Subgraph_StdDev2Pos.DrawStyle = DRAWSTYLE_IGNORE; 
		Subgraph_StdDev2Pos.PrimaryColor = RGB(0, 255, 0);
		
		Subgraph_StdDev2Neg.Name = "-1.0 StdDev";
		Subgraph_StdDev2Neg.DrawStyle = DRAWSTYLE_IGNORE; 
		Subgraph_StdDev2Neg.PrimaryColor = RGB(255, 0, 0);

		Subgraph_StdDev3Pos.Name = "+1.5 StdDev";
		Subgraph_StdDev3Pos.DrawStyle = DRAWSTYLE_IGNORE; 
		Subgraph_StdDev3Pos.PrimaryColor = RGB(0, 255, 0);
		
		Subgraph_StdDev3Neg.Name = "-1.5 StdDev";
		Subgraph_StdDev3Neg.DrawStyle = DRAWSTYLE_IGNORE; 
		Subgraph_StdDev3Neg.PrimaryColor = RGB(255, 0, 0);

		Input_ResetTime.Name = "Daily Reset Time";
		Input_ResetTime.SetTime(HMS_TIME(17, 0, 0)); // Default 17:00:00
		
		Input_AlertSound.Name = "Alert Sound Number";
		Input_AlertSound.SetInt(0); // 0 = Off
		
		Input_AlertMessage.Name = "Alert Message";
		Input_AlertMessage.SetString("Daily AVWAP Crossover");
		
		Input_StdDev1Mult.Name = "StdDev Band 1 Multiplier";
		Input_StdDev1Mult.SetFloat(0.5f);
		
		Input_StdDev2Mult.Name = "StdDev Band 2 Multiplier";
		Input_StdDev2Mult.SetFloat(1.0f);
		
		Input_StdDev3Mult.Name = "StdDev Band 3 Multiplier";
		Input_StdDev3Mult.SetFloat(1.5f);

		return;
	}

	// Optimization: Fetch Inputs ONCE per update cycle, not per bar.
	double ResetTimeDouble = Input_ResetTime.GetDateTime().GetTime();
	int AlertSoundIdx = Input_AlertSound.GetInt();
	SCString AlertMsg = Input_AlertMessage.GetString();
	float StdDev1Mult = Input_StdDev1Mult.GetFloat();
	float StdDev2Mult = Input_StdDev2Mult.GetFloat();
	float StdDev3Mult = Input_StdDev3Mult.GetFloat();

	int& LastAlertBarIndex = sc.GetPersistentInt(1);

	// ---------------------------------------------------------------
	// Double-precision accumulator arrays via persistent pointer.
	//
	// We accumulate PV, Vol, and DevSqVol in double precision.
	// DevSqVol = Σ( V_i × (P_i - VWAP_i)² ) where VWAP_i is the
	// running VWAP up to bar i. This matches Sierra Chart's built-in
	// VWAP standard deviation calculation, which measures each bar's
	// deviation from the running VWAP at that point in time.
	// ---------------------------------------------------------------
	struct DoubleAccumulators
	{
		int AllocatedSize;
		double* RunningPV;
		double* RunningVol;
		double* RunningDevSqVol; // Σ( V_i × (P_i - VWAP_i)² )
	};

	DoubleAccumulators*& p_Accum = (DoubleAccumulators*&)sc.GetPersistentPointer(1);

	// Allocate or reallocate if array size changed
	if (p_Accum == NULL || p_Accum->AllocatedSize < sc.ArraySize)
	{
		// Free old memory if resizing
		if (p_Accum != NULL)
		{
			sc.FreeMemory(p_Accum->RunningPV);
			sc.FreeMemory(p_Accum->RunningVol);
			sc.FreeMemory(p_Accum->RunningDevSqVol);
			sc.FreeMemory(p_Accum);
		}

		int AllocSize = sc.ArraySize + 1000; // Over-allocate to reduce reallocs

		p_Accum = (DoubleAccumulators*)sc.AllocateMemory(sizeof(DoubleAccumulators));
		p_Accum->RunningPV = (double*)sc.AllocateMemory(AllocSize * sizeof(double));
		p_Accum->RunningVol = (double*)sc.AllocateMemory(AllocSize * sizeof(double));
		p_Accum->RunningDevSqVol = (double*)sc.AllocateMemory(AllocSize * sizeof(double));
		p_Accum->AllocatedSize = AllocSize;

		// Zero out - must recalculate from scratch
		memset(p_Accum->RunningPV, 0, AllocSize * sizeof(double));
		memset(p_Accum->RunningVol, 0, AllocSize * sizeof(double));
		memset(p_Accum->RunningDevSqVol, 0, AllocSize * sizeof(double));

		// Force full recalc when memory is re-allocated
		sc.UpdateStartIndex = 0;
	}

	// Manual Loop Implementation
	int StartIndex = sc.UpdateStartIndex;

	for (int i = StartIndex; i < sc.ArraySize; i++)
	{
		// Data
		float Close = sc.BaseData[SC_LAST][i];
		float Volume = sc.BaseData[SC_VOLUME][i];
		
		// Use Last/Close price (matches Sierra Chart built-in VWAP default)
		double Price = (double)Close;
		
		double CurrentPV = Price * (double)Volume;
		double CurrentVol = (double)Volume;

		// Reset Logic
		bool DoReset = false;

		if (i == 0)
		{
			DoReset = true;
		}
		else
		{
			// Safe/Robust Anchor Calculation Logic
			
			// 1. Current Bar Anchor Logic
			SCDateTime CurrentDT = sc.BaseDateTimeIn[i];
			SCDateTime CurrentDate = CurrentDT.GetDate();
			double CurrentTime = CurrentDT.GetTime();
			
			// Determine which "Anchor Day" this bar belongs to.
			// If Time >= ResetTime, it belongs to Today's Anchor.
			// If Time < ResetTime, it belongs to Yesterday's Anchor.
			SCDateTime CurrentAnchorBaseDate = CurrentDate;
			
			if (CurrentTime < ResetTimeDouble)
			{
				CurrentAnchorBaseDate -= 1.0; // Go back 1 day
			}
			SCDateTime CurrentAnchor = CurrentAnchorBaseDate + ResetTimeDouble;

			// 2. Previous Bar Anchor Logic
			SCDateTime PreviousDT = sc.BaseDateTimeIn[i - 1];
			SCDateTime PreviousDate = PreviousDT.GetDate();
			double PreviousTime = PreviousDT.GetTime();

			SCDateTime PreviousAnchorBaseDate = PreviousDate;
			if (PreviousTime < ResetTimeDouble)
			{
				PreviousAnchorBaseDate -= 1.0;
			}
			SCDateTime PreviousAnchor = PreviousAnchorBaseDate + ResetTimeDouble;

			// 3. Comparison
			if (CurrentAnchor > PreviousAnchor)
			{
				DoReset = true;
			}
		}

		if (DoReset)
		{
			p_Accum->RunningPV[i] = CurrentPV; 
			p_Accum->RunningVol[i] = CurrentVol;
			// On reset, VWAP_i = Price, so deviation = 0
			p_Accum->RunningDevSqVol[i] = 0.0;
		}
		else
		{
			// Accumulate PV and Vol first to get running VWAP
			p_Accum->RunningPV[i] = p_Accum->RunningPV[i - 1] + CurrentPV;
			p_Accum->RunningVol[i] = p_Accum->RunningVol[i - 1] + CurrentVol;
			
			// Compute deviation from the running VWAP at this bar
			// This matches Sierra Chart's built-in method
			double RunningVWAP = p_Accum->RunningPV[i] / p_Accum->RunningVol[i];
			double Deviation = Price - RunningVWAP;
			p_Accum->RunningDevSqVol[i] = p_Accum->RunningDevSqVol[i - 1] + (CurrentVol * Deviation * Deviation);
		}

		// Calculate VWAP
		if (p_Accum->RunningVol[i] != 0.0)
		{
			double VWAP = p_Accum->RunningPV[i] / p_Accum->RunningVol[i];
			Subgraph_VWAP[i] = (float)VWAP;
			
			// Standard Deviation: sqrt( Σ(V_i × (P_i - VWAP_i)²) / Σ(V_i) )
			// Each bar's deviation is measured from the running VWAP at that bar
			double Variance = p_Accum->RunningDevSqVol[i] / p_Accum->RunningVol[i];
			
			if (Variance < 0.0) Variance = 0.0;
			
			double StdDev = sqrt(Variance);
			
			Subgraph_StdDev1Pos[i] = (float)(VWAP + (StdDev1Mult * StdDev));
			Subgraph_StdDev1Neg[i] = (float)(VWAP - (StdDev1Mult * StdDev));
			Subgraph_StdDev2Pos[i] = (float)(VWAP + (StdDev2Mult * StdDev));
			Subgraph_StdDev2Neg[i] = (float)(VWAP - (StdDev2Mult * StdDev));
			Subgraph_StdDev3Pos[i] = (float)(VWAP + (StdDev3Mult * StdDev));
			Subgraph_StdDev3Neg[i] = (float)(VWAP - (StdDev3Mult * StdDev));
			
			// Alert Logic - Only check LAST BAR, deduplicate per bar index
			if (i == sc.ArraySize - 1 && i > 0 && AlertSoundIdx > 0 && i != LastAlertBarIndex)
			{
				float LastPrice = sc.Close[i];
				float PrevPrice = sc.Close[i-1];
				
				// Indices 0-6 correspond to VWAP, +1, -1, +2, -2, +3, -3
				for (int k = 0; k < 7; k++)
				{
					// Skip bands that are not visible
					if (sc.Subgraph[k].DrawStyle == DRAWSTYLE_IGNORE)
						continue;
					
					float LineCurr = sc.Subgraph[k][i];
					float LinePrev = sc.Subgraph[k][i-1];
					
					bool CrossedAbove = (PrevPrice < LinePrev && LastPrice >= LineCurr);
					bool CrossedBelow = (PrevPrice > LinePrev && LastPrice <= LineCurr);
					
					if (CrossedAbove || CrossedBelow)
					{
						sc.SetAlert(AlertSoundIdx, AlertMsg);
						LastAlertBarIndex = i;
						break; 
					}
				}
			}
		}
		else
		{
			if (i > 0)
			{
				Subgraph_VWAP[i] = Subgraph_VWAP[i - 1]; 
				Subgraph_StdDev1Pos[i] = Subgraph_StdDev1Pos[i - 1];
				Subgraph_StdDev1Neg[i] = Subgraph_StdDev1Neg[i - 1];
				Subgraph_StdDev2Pos[i] = Subgraph_StdDev2Pos[i - 1];
				Subgraph_StdDev2Neg[i] = Subgraph_StdDev2Neg[i - 1];
				Subgraph_StdDev3Pos[i] = Subgraph_StdDev3Pos[i - 1];
				Subgraph_StdDev3Neg[i] = Subgraph_StdDev3Neg[i - 1];
			}
			// else: i == 0 with zero volume, leave as 0
		}
	}
}

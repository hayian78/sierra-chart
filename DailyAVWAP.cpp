#include "sierrachart.h"
SCDLLName("DailyAVWAP")
static const char* VERSION = "1.4.0";

SCSFExport scsf_DailyAVWAP(SCStudyInterfaceRef sc)
{
	// Subgraphs
	SCSubgraphRef Subgraph_VWAP = sc.Subgraph[0];
	SCSubgraphRef Subgraph_StdDev1Pos = sc.Subgraph[1];
	SCSubgraphRef Subgraph_StdDev1Neg = sc.Subgraph[2];
	SCSubgraphRef Subgraph_StdDev2Pos = sc.Subgraph[3];
	SCSubgraphRef Subgraph_StdDev2Neg = sc.Subgraph[4];
	SCSubgraphRef Subgraph_StdDev3Pos = sc.Subgraph[5];
	SCSubgraphRef Subgraph_StdDev3Neg = sc.Subgraph[6];
	SCSubgraphRef Subgraph_StdDev4Pos = sc.Subgraph[7];
	SCSubgraphRef Subgraph_StdDev4Neg = sc.Subgraph[8];
	SCSubgraphRef Subgraph_StdDev5Pos = sc.Subgraph[9];
	SCSubgraphRef Subgraph_StdDev5Neg = sc.Subgraph[10];

	// Inputs
	SCInputRef Input_ResetTime = sc.Input[0];
	SCInputRef Input_AlertSound = sc.Input[1];
	SCInputRef Input_AlertMessage = sc.Input[2];
	SCInputRef Input_StdDev1Mult = sc.Input[3];
	SCInputRef Input_StdDev2Mult = sc.Input[4];
	SCInputRef Input_StdDev3Mult = sc.Input[5];
	SCInputRef Input_StdDev4Mult = sc.Input[6];
	SCInputRef Input_StdDev5Mult = sc.Input[7];

	if (sc.SetDefaults)
	{
		sc.GraphName = "Daily AVWAP";
		sc.GraphShortName = "DAVWAP";
		sc.StudyDescription = "Daily AVWAP v1.4.0 - Calculates VWAP anchored daily at a configurable reset time. Supports 5 standard deviation bands with customizable multipliers.";

		sc.GraphRegion = 0; 
		sc.AutoLoop = 0; 
		sc.ValueFormat = VALUEFORMAT_INHERITED;

		Subgraph_VWAP.Name = "VWAP";
		Subgraph_VWAP.DrawStyle = DRAWSTYLE_LINE;
		Subgraph_VWAP.LineWidth = 2;
		Subgraph_VWAP.PrimaryColor = RGB(255, 0, 255); // Magenta
		Subgraph_VWAP.DrawZeros = false;
		
		Subgraph_StdDev1Pos.Name = "STD 1 Pos";
		Subgraph_StdDev1Pos.DrawStyle = DRAWSTYLE_IGNORE; 
		Subgraph_StdDev1Pos.PrimaryColor = RGB(0, 255, 0);
		
		Subgraph_StdDev1Neg.Name = "STD 1 Neg";
		Subgraph_StdDev1Neg.DrawStyle = DRAWSTYLE_IGNORE; 
		Subgraph_StdDev1Neg.PrimaryColor = RGB(255, 0, 0);

		Subgraph_StdDev2Pos.Name = "STD 2 Pos";
		Subgraph_StdDev2Pos.DrawStyle = DRAWSTYLE_IGNORE; 
		Subgraph_StdDev2Pos.PrimaryColor = RGB(0, 255, 0);
		
		Subgraph_StdDev2Neg.Name = "STD 2 Neg";
		Subgraph_StdDev2Neg.DrawStyle = DRAWSTYLE_IGNORE; 
		Subgraph_StdDev2Neg.PrimaryColor = RGB(255, 0, 0);

		Subgraph_StdDev3Pos.Name = "STD 3 Pos";
		Subgraph_StdDev3Pos.DrawStyle = DRAWSTYLE_IGNORE; 
		Subgraph_StdDev3Pos.PrimaryColor = RGB(0, 255, 0);
		
		Subgraph_StdDev3Neg.Name = "STD 3 Neg";
		Subgraph_StdDev3Neg.DrawStyle = DRAWSTYLE_IGNORE; 
		Subgraph_StdDev3Neg.PrimaryColor = RGB(255, 0, 0);

		Subgraph_StdDev4Pos.Name = "STD 4 Pos";
		Subgraph_StdDev4Pos.DrawStyle = DRAWSTYLE_IGNORE; 
		Subgraph_StdDev4Pos.PrimaryColor = RGB(0, 255, 0);
		
		Subgraph_StdDev4Neg.Name = "STD 4 Neg";
		Subgraph_StdDev4Neg.DrawStyle = DRAWSTYLE_IGNORE; 
		Subgraph_StdDev4Neg.PrimaryColor = RGB(255, 0, 0);

		Subgraph_StdDev5Pos.Name = "STD 5 Pos";
		Subgraph_StdDev5Pos.DrawStyle = DRAWSTYLE_IGNORE; 
		Subgraph_StdDev5Pos.PrimaryColor = RGB(0, 255, 0);
		
		Subgraph_StdDev5Neg.Name = "STD 5 Neg";
		Subgraph_StdDev5Neg.DrawStyle = DRAWSTYLE_IGNORE; 
		Subgraph_StdDev5Neg.PrimaryColor = RGB(255, 0, 0);

		Input_ResetTime.Name = "Daily Reset Time";
		Input_ResetTime.SetTime(HMS_TIME(17, 0, 0)); 
		
		Input_AlertSound.Name = "Alert Sound Number";
		Input_AlertSound.SetInt(0); 
		
		Input_AlertMessage.Name = "Alert Message";
		Input_AlertMessage.SetString("Daily AVWAP Crossover");
		
		Input_StdDev1Mult.Name = "StdDev Band 1 Multiplier";
		Input_StdDev1Mult.SetFloat(1.0f);
		
		Input_StdDev2Mult.Name = "StdDev Band 2 Multiplier";
		Input_StdDev2Mult.SetFloat(2.0f);
		
		Input_StdDev3Mult.Name = "StdDev Band 3 Multiplier";
		Input_StdDev3Mult.SetFloat(3.0f);

		Input_StdDev4Mult.Name = "StdDev Band 4 Multiplier";
		Input_StdDev4Mult.SetFloat(4.0f);

		Input_StdDev5Mult.Name = "StdDev Band 5 Multiplier";
		Input_StdDev5Mult.SetFloat(5.0f);

		return;
	}

	double ResetTimeDouble = Input_ResetTime.GetDateTime().GetTime();
	int AlertSoundIdx = Input_AlertSound.GetInt();
	SCString AlertMsg = Input_AlertMessage.GetString();
	float StdDev1Mult = Input_StdDev1Mult.GetFloat();
	float StdDev2Mult = Input_StdDev2Mult.GetFloat();
	float StdDev3Mult = Input_StdDev3Mult.GetFloat();
	float StdDev4Mult = Input_StdDev4Mult.GetFloat();
	float StdDev5Mult = Input_StdDev5Mult.GetFloat();

	int& LastAlertBarIndex = sc.GetPersistentInt(1);

	struct DoubleAccumulators
	{
		int AllocatedSize;
		double* RunningPV;
		double* RunningVol;
		double* RunningDevSqVol; 
	};

	DoubleAccumulators*& p_Accum = (DoubleAccumulators*&)sc.GetPersistentPointer(1);

	if (p_Accum == NULL || p_Accum->AllocatedSize < sc.ArraySize)
	{
		if (p_Accum != NULL)
		{
			sc.FreeMemory(p_Accum->RunningPV);
			sc.FreeMemory(p_Accum->RunningVol);
			sc.FreeMemory(p_Accum->RunningDevSqVol);
			sc.FreeMemory(p_Accum);
		}

		int AllocSize = sc.ArraySize + 1000; 

		p_Accum = (DoubleAccumulators*)sc.AllocateMemory(sizeof(DoubleAccumulators));
		p_Accum->RunningPV = (double*)sc.AllocateMemory(AllocSize * sizeof(double));
		p_Accum->RunningVol = (double*)sc.AllocateMemory(AllocSize * sizeof(double));
		p_Accum->RunningDevSqVol = (double*)sc.AllocateMemory(AllocSize * sizeof(double));
		p_Accum->AllocatedSize = AllocSize;

		memset(p_Accum->RunningPV, 0, AllocSize * sizeof(double));
		memset(p_Accum->RunningVol, 0, AllocSize * sizeof(double));
		memset(p_Accum->RunningDevSqVol, 0, AllocSize * sizeof(double));

		sc.UpdateStartIndex = 0;
	}

	int StartIndex = sc.UpdateStartIndex;

	for (int i = StartIndex; i < sc.ArraySize; i++)
	{
		float Close = sc.BaseData[SC_LAST][i];
		float Volume = sc.BaseData[SC_VOLUME][i];
		double Price = (double)Close;
		double CurrentPV = Price * (double)Volume;
		double CurrentVol = (double)Volume;

		bool DoReset = false;
		if (i == 0)
		{
			DoReset = true;
		}
		else
		{
			SCDateTime CurrentDT = sc.BaseDateTimeIn[i];
			SCDateTime CurrentDate = CurrentDT.GetDate();
			double CurrentTime = CurrentDT.GetTime();
			
			SCDateTime CurrentAnchorBaseDate = CurrentDate;
			if (CurrentTime < ResetTimeDouble)
				CurrentAnchorBaseDate -= 1.0; 
			SCDateTime CurrentAnchor = CurrentAnchorBaseDate + ResetTimeDouble;

			SCDateTime PreviousDT = sc.BaseDateTimeIn[i - 1];
			SCDateTime PreviousDate = PreviousDT.GetDate();
			double PreviousTime = PreviousDT.GetTime();

			SCDateTime PreviousAnchorBaseDate = PreviousDate;
			if (PreviousTime < ResetTimeDouble)
				PreviousAnchorBaseDate -= 1.0;
			SCDateTime PreviousAnchor = PreviousAnchorBaseDate + ResetTimeDouble;

			if (CurrentAnchor > PreviousAnchor)
				DoReset = true;
		}

		if (DoReset)
		{
			p_Accum->RunningPV[i] = CurrentPV; 
			p_Accum->RunningVol[i] = CurrentVol;
			p_Accum->RunningDevSqVol[i] = 0.0;
		}
		else
		{
			p_Accum->RunningPV[i] = p_Accum->RunningPV[i - 1] + CurrentPV;
			p_Accum->RunningVol[i] = p_Accum->RunningVol[i - 1] + CurrentVol;
			
			double RunningVWAP = p_Accum->RunningPV[i] / p_Accum->RunningVol[i];
			double Deviation = Price - RunningVWAP;
			p_Accum->RunningDevSqVol[i] = p_Accum->RunningDevSqVol[i - 1] + (CurrentVol * Deviation * Deviation);
		}

		if (p_Accum->RunningVol[i] != 0.0)
		{
			double VWAP = p_Accum->RunningPV[i] / p_Accum->RunningVol[i];
			Subgraph_VWAP[i] = (float)VWAP;
			
			double Variance = p_Accum->RunningDevSqVol[i] / p_Accum->RunningVol[i];
			if (Variance < 0.0) Variance = 0.0;
			double StdDev = sqrt(Variance);
			
			Subgraph_StdDev1Pos[i] = (float)(VWAP + (StdDev1Mult * StdDev));
			Subgraph_StdDev1Neg[i] = (float)(VWAP - (StdDev1Mult * StdDev));
			Subgraph_StdDev2Pos[i] = (float)(VWAP + (StdDev2Mult * StdDev));
			Subgraph_StdDev2Neg[i] = (float)(VWAP - (StdDev2Mult * StdDev));
			Subgraph_StdDev3Pos[i] = (float)(VWAP + (StdDev3Mult * StdDev));
			Subgraph_StdDev3Neg[i] = (float)(VWAP - (StdDev3Mult * StdDev));
			Subgraph_StdDev4Pos[i] = (float)(VWAP + (StdDev4Mult * StdDev));
			Subgraph_StdDev4Neg[i] = (float)(VWAP - (StdDev4Mult * StdDev));
			Subgraph_StdDev5Pos[i] = (float)(VWAP + (StdDev5Mult * StdDev));
			Subgraph_StdDev5Neg[i] = (float)(VWAP - (StdDev5Mult * StdDev));
			
			// Alert Logic - Updated for 11 subgraphs (VWAP + 5 pairs)
			if (i == sc.ArraySize - 1 && i > 0 && AlertSoundIdx > 0 && i != LastAlertBarIndex)
			{
				float LastPrice = sc.Close[i];
				float PrevPrice = sc.Close[i-1];
				
				for (int k = 0; k < 11; k++)
				{
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
				Subgraph_StdDev4Pos[i] = Subgraph_StdDev4Pos[i - 1];
				Subgraph_StdDev4Neg[i] = Subgraph_StdDev4Neg[i - 1];
				Subgraph_StdDev5Pos[i] = Subgraph_StdDev5Pos[i - 1];
				Subgraph_StdDev5Neg[i] = Subgraph_StdDev5Neg[i - 1];
			}
		}
	}
}

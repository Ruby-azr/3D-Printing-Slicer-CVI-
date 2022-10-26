#include <cvintwrk.h>
#include "toolbox.h"
#include <formatio.h>
#include <analysis.h>
#include <ansi_c.h>
#include <cvirte.h>
#include <userint.h>
#include "Slicer.h"

static int panelHandle,panelHandle2,panelHandle3;

FILE *fp;
char filename[260], fileLine[200];
int polyCounter, totalLayers;
float layerT, modelH, xMin, xMax, yMin, yMax, zMin;
int bedTemp, nozzleTemp, printSpeed;
float xOffset, yOffset, xMid, yMid, bedlength = 180, bedwidth = 180, modelWidth, modelLength;
int currentLayer=1;
double lastScaleFactor, scaleFactor=1;
int counter, top,left;


typedef struct
{
	float x;
	float y;
	float z;
} vertex;

typedef struct
{
	vertex vertex[3];
} polygon;

polygon* poly;


typedef struct
{
	float x_arr[10000]; 
	float y_arr[10000]; 
	int numOfCoor;
	int numOfSeg;
	float interMat[10000][4];  

} layers;

layers *layer;

//-----------------------------------------------------------------------------------------------------------
int main (int argc, char *argv[])
{
	if (InitCVIRTE (0, argv, 0) == 0)
		return -1;	/* out of memory */
	if ((panelHandle = LoadPanel (0, "Slicer.uir", PANEL)) < 0)
		return -1;
	if ((panelHandle2 = LoadPanel (0, "Slicer.uir", PANEL_SIMU)) < 0)
		return -1;
	if ((panelHandle3 = LoadPanel (0, "Slicer.uir", PANEL_ABOU)) < 0)
		return -1;

	DisplayPanel (panelHandle);
	RunUserInterface ();
	DiscardPanel (panelHandle);
	DiscardPanel (panelHandle2);
	DiscardPanel (panelHandle3);
	free(poly);
	free(layer);
	return 0;
}
//-----------------------------------------------------------------------------------------------------------
int CVICALLBACK quitFunction (int panel, int event, void *callbackData,
							  int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_GOT_FOCUS:

			break;
		case EVENT_LOST_FOCUS:

			break;
		case EVENT_CLOSE:
			QuitUserInterface (0);
			break;
	}
	return 0;
}
//-----------------------------------------------------------------------------------------------------------
int CVICALLBACK quitSimulateFunc (int panel, int event, void *callbackData,
								  int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_GOT_FOCUS:

			break;
		case EVENT_LOST_FOCUS:

			break;
		case EVENT_CLOSE:
			HidePanel(panelHandle2);
			SetCtrlAttribute (panelHandle2, PANEL_SIMU_TIMER, ATTR_ENABLED, 0);
			DeleteGraphPlot (panelHandle2, PANEL_SIMU_GRAPH_SIMULATION, -1, VAL_IMMEDIATE_DRAW);
			SetCtrlAttribute (panelHandle2, PANEL_SIMU_BUTTON_DRAW, ATTR_CTRL_MODE, VAL_HOT);
			SetCtrlAttribute (panelHandle2, PANEL_SIMU_PICTURE, ATTR_TOP, 290);
			SetCtrlAttribute (panelHandle2, PANEL_SIMU_PICTURE, ATTR_LEFT, 670);
			break;
	}
	return 0;
}
//-----------------------------------------------------------------------------------------------------------
int CVICALLBACK quitAboutFunc (int panel, int event, void *callbackData,
							   int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_GOT_FOCUS:

			break;
		case EVENT_LOST_FOCUS:

			break;
		case EVENT_CLOSE:
			HidePanel(panelHandle3);
			break;
	}
	return 0;
}
//-----------------------------------------------------------------------------------------------------------
void initSlider()
{
	SetCtrlAttribute (panelHandle, PANEL_NUMERIC_SLIDER, ATTR_VISIBLE, 1);
	SetCtrlAttribute (panelHandle, PANEL_NUMERIC_SLIDER, ATTR_MAX_VALUE, totalLayers);
	SetCtrlAttribute (panelHandle, PANEL_NUMERIC_SLIDER, ATTR_CTRL_VAL, 0);
}
//-----------------------------------------------------------------------------------------------------------
void sortFunction(int a)
{
	int i=0, flag=0, newContour=1;

	while(newContour==1)
	{
		newContour = 0;
		//search for a new close contour
		for(int j=0; j<layer[a].numOfCoor; j++)
		{
			if(layer[a].interMat[j][2]!=0)
			{
				newContour = 1;  //there is a new contour
				flag = 1;  		 //this is for the sorting loop

				//save it
				layer[a].x_arr[i] = layer[a].interMat[j][0];
				layer[a].y_arr[i] = layer[a].interMat[j][1];
				layer[a].interMat[j][2] = 0;

				if(layer[a].interMat[j][3]==1) //if the eqution is with point of "index" = 1
				{
					//save the values that in "index" 2
					layer[a].x_arr[i+1] = layer[a].interMat[j+1][0];
					layer[a].y_arr[i+1] = layer[a].interMat[j+1][1];
					layer[a].interMat[j][2]=0;
					layer[a].interMat[j+1][2]=0;
				}
				else   //the eqution is with point of "index" = 2
				{
					//save the values that in "index" 1
					layer[a].x_arr[i+1] = layer[a].interMat[j-1][0];
					layer[a].y_arr[i+1] = layer[a].interMat[j-1][1];
					layer[a].interMat[j][2]=0;
					layer[a].interMat[j-1][2]=0;
				}

				i++;
				break;
			}
		}

		//the sorting loop
		while(flag==1) //at first time it happan only if there is a new contour
		{
			flag = 0;

			for(int j=2; j<layer[a].numOfCoor; j++)	   //running on the matrix
			{
				if((layer[a].x_arr[i]==layer[a].interMat[j][0])&&(layer[a].y_arr[i]==layer[a].interMat[j][1])&&(layer[a].interMat[j][2]!=0))
				{
					//there is a match
					flag = 1;

					if(layer[a].interMat[j][3]==1) //if the eqution is with point of "index" = 1
					{
						//save the values that in "index" 2
						layer[a].x_arr[i+1] = layer[a].interMat[j+1][0];
						layer[a].y_arr[i+1] = layer[a].interMat[j+1][1];
						layer[a].interMat[j][2]=0;
						layer[a].interMat[j+1][2]=0;
					}
					else  //the eqution is with point of "index" = 2
					{
						//save the values that in "index" 1
						layer[a].x_arr[i+1] = layer[a].interMat[j-1][0];
						layer[a].y_arr[i+1] = layer[a].interMat[j-1][1];
						layer[a].interMat[j][2]=0;
						layer[a].interMat[j-1][2]=0;
					}

					break; //because we found a match, need to break the for loop

				}//in case we dont find a match, we need keep running the matrix in the for loop

			}//-end of for loop----------------------------------------------------------

			i++;
			layer[a].numOfSeg = i;
		}
	}
}
//---------------------------------------------------------------------------------------------------------//
//---------------- Checking the model height- the max z value. Also the min and max of x,y ----------------//
//---------------------------------------------------------------------------------------------------------//
void modelHeightFunc()
{
	char garbage[100];

	fp = fopen (filename, "r");
	fgets (fileLine, 199, fp);	//first line

	int i = 0;
	for(int j=0; j < polyCounter; j++)
	{
		fgets (fileLine, 199, fp);	 //"facet normal" line
		fgets (fileLine, 199, fp);	 //"outerloop" line


		fgets (fileLine, 199, fp);	//vertex 1
		sscanf (fileLine, "%s %f %f %f", garbage, &poly[i].vertex[0].x, &poly[i].vertex[0].y, &poly[i].vertex[0].z);		//saving x y z for vertex 1
		if(poly[i].vertex[0].z > modelH)
			modelH = poly[i].vertex[0].z;

		//finding max
		if(poly[i].vertex[0].x > xMax)
			xMax = poly[i].vertex[0].x;
		if(poly[i].vertex[0].y > yMax)
			yMax = poly[i].vertex[0].y;
		//finding min
		if(poly[i].vertex[0].x < xMin)
			xMin = poly[i].vertex[0].x;
		if(poly[i].vertex[0].y < yMin)
			yMin = poly[i].vertex[0].y;
		if(poly[i].vertex[0].z < zMin)
			zMin = poly[i].vertex[0].z;


		fgets (fileLine, 199, fp);	//vertex 2
		sscanf (fileLine, "%s %f %f %f", garbage, &poly[i].vertex[1].x, &poly[i].vertex[1].y, &poly[i].vertex[1].z);		//saving x y z for vertex 2
		if(poly[i].vertex[1].z > modelH)
			modelH = poly[i].vertex[1].z;

		//finding max
		if(poly[i].vertex[1].x > xMax)
			xMax = poly[i].vertex[1].x;
		if(poly[i].vertex[1].y > yMax)
			yMax = poly[i].vertex[1].y;
		//finding min
		if(poly[i].vertex[1].x < xMin)
			xMin = poly[i].vertex[1].x;
		if(poly[i].vertex[1].y < yMin)
			yMin = poly[i].vertex[1].y;
		if(poly[i].vertex[1].z < zMin)
			zMin = poly[i].vertex[1].z;


		fgets (fileLine, 199, fp);	//vertex 3
		sscanf (fileLine, "%s %f %f %f", garbage, &poly[i].vertex[2].x, &poly[i].vertex[2].y, &poly[i].vertex[2].z);		//saving x y z for vertex 3
		if(poly[i].vertex[2].z > modelH)
			modelH = poly[i].vertex[2].z;

		//finding max
		if(poly[i].vertex[2].x > xMax)
			xMax = poly[i].vertex[2].x;
		if(poly[i].vertex[2].y > yMax)
			yMax = poly[i].vertex[2].y;
		//finding min
		if(poly[i].vertex[2].x < xMin)
			xMin = poly[i].vertex[2].x;
		if(poly[i].vertex[2].y < yMin)
			yMin = poly[i].vertex[2].y;
		if(poly[i].vertex[2].z < zMin)
			zMin = poly[i].vertex[2].z;

		if((poly[i].vertex[0].z==poly[i].vertex[1].z)&&(poly[i].vertex[0].z==poly[i].vertex[2].z)&&(poly[i].vertex[1].z==poly[i].vertex[2].z))
		{
			i=i-1;					   //for not saving triangles with same Z
		}
		i++;

		fgets (fileLine, 199, fp);	//"endloop" line
		fgets (fileLine, 199, fp);	//"endfacet" line
	}

	if(zMin>0) //for placing the model at the right height- in case zMin is not at 0.00
	{
		for(int k=0; k < polyCounter; k++)
		{
			poly[k].vertex[0].z =  poly[k].vertex[0].z - zMin;
			poly[k].vertex[1].z =  poly[k].vertex[1].z - zMin;
			poly[k].vertex[2].z =  poly[k].vertex[2].z - zMin;

		}
	}

	modelWidth = xMax-xMin;
	modelLength = yMax-yMin;
	polyCounter = i;

	fclose (fp);

}
//---------------------------------------------------------------------------------------------------------//
//---------------------------- Checking the number of polygons in the stl file ----------------------------//
//---------------------------------------------------------------------------------------------------------//
void findPolyNumFunc()
{
	char fileLine2[200], str[200]= {' ', '\0'};

	fgets (fileLine2, 199, fp);
	fgets (fileLine2, 199, fp);		//getting the "outer loop" string in line 3
	strcpy (str, fileLine2);	    //coping the "outer loop" string to "str"
	polyCounter = 1;

	do
	{
		fgets (fileLine2, 199, fp);
		if(strcmp (fileLine2,str)==0)
		{
			polyCounter++;
		}
	}
	while(strcmp (fileLine2,"endsolid")!=0);
	fclose (fp);
}

//---------------------------------------------------------------------------------------------------------//
//----------------------------------                                      ---------------------------------//
//---------------------------------------------------------------------------------------------------------//
void intersectionFunc(int a, int p)
{
	int countZequal=0, countZup=0, countZDown=0;
	int arrayUp[3]= {0,0,0}, arrayDown[3]= {0,0,0}, arrayEqual[3]= {0,0,0};
	int indexVer, indexVer1, indexVer2, temp, j=0, n=0, k=0;
	float zPlane;

	zPlane = layerT*a;

//running on the 3 vertices and counting how many in/above/beneath the current plane & saving the index of the vertex
	for(int i=0; i<3 ; i++)
	{
		if(poly[p].vertex[i].z >= zPlane)
		{
			if(poly[p].vertex[i].z <= zPlane)
			{
				//zi = zp
				countZequal++;
				arrayEqual[j] = i;
				j++;
			}
			else
			{
				//zi > zp
				countZup++;
				arrayUp[n]=i;
				n++;
			}
		}
		else
		{
			//zi<zp
			countZDown++;
			arrayDown[k]=i;
			k++;
		}
	}

//------------- sorting the orientation type of the polygon & computing the
//---------- intersection coortinates of the polygon with the current Z plane

	if(countZequal==0)
	{
		if((countZDown==0) || (countZup==0))
		{
			//no intersection with this triangle
			return;
		}
		else	 	//intersection with 2 facets
		{
			if(countZDown==2)		//1 above 2 beneath
			{
				indexVer1 = arrayDown[0];
				indexVer2 = arrayUp[0];
				temp = layer[a].numOfCoor;
				//x1
				layer[a].interMat[temp][0] = (zPlane-poly[p].vertex[indexVer2].z)*(poly[p].vertex[indexVer2].x-poly[p].vertex[indexVer1].x)/(poly[p].vertex[indexVer2].z-poly[p].vertex[indexVer1].z)+poly[p].vertex[indexVer2].x+xOffset;
				//y1
				layer[a].interMat[temp][1] = (zPlane-poly[p].vertex[indexVer2].z)*(poly[p].vertex[indexVer2].y-poly[p].vertex[indexVer1].y)/(poly[p].vertex[indexVer2].z-poly[p].vertex[indexVer1].z)+poly[p].vertex[indexVer2].y+yOffset;
				layer[a].interMat[temp][2] = layer[a].numOfSeg;
				layer[a].interMat[temp][3] = 1;
				layer[a].numOfCoor++;

				indexVer1 = arrayDown[1];
				indexVer2 = arrayUp[0];
				temp = layer[a].numOfCoor;
				//x2
				layer[a].interMat[temp][0] = (zPlane-poly[p].vertex[indexVer2].z)*(poly[p].vertex[indexVer2].x-poly[p].vertex[indexVer1].x)/(poly[p].vertex[indexVer2].z-poly[p].vertex[indexVer1].z)+poly[p].vertex[indexVer2].x+xOffset;
				//y2
				layer[a].interMat[temp][1] = (zPlane-poly[p].vertex[indexVer2].z)*(poly[p].vertex[indexVer2].y-poly[p].vertex[indexVer1].y)/(poly[p].vertex[indexVer2].z-poly[p].vertex[indexVer1].z)+poly[p].vertex[indexVer2].y+yOffset;
				layer[a].interMat[temp][2] = layer[a].numOfSeg;
				layer[a].interMat[temp][3] = 2;
				layer[a].numOfCoor++;
				layer[a].numOfSeg++;
			}
			else	//1 beneath 2 above
			{
				indexVer1 = arrayDown[0];
				indexVer2 = arrayUp[0];
				temp = layer[a].numOfCoor;
				//x1
				layer[a].interMat[temp][0] = (zPlane-poly[p].vertex[indexVer2].z)*(poly[p].vertex[indexVer2].x-poly[p].vertex[indexVer1].x)/(poly[p].vertex[indexVer2].z-poly[p].vertex[indexVer1].z)+poly[p].vertex[indexVer2].x+xOffset;
				//y1
				layer[a].interMat[temp][1] = (zPlane-poly[p].vertex[indexVer2].z)*(poly[p].vertex[indexVer2].y-poly[p].vertex[indexVer1].y)/(poly[p].vertex[indexVer2].z-poly[p].vertex[indexVer1].z)+poly[p].vertex[indexVer2].y+yOffset;
				layer[a].interMat[temp][2] = layer[a].numOfSeg;
				layer[a].interMat[temp][3] = 1;
				layer[a].numOfCoor++;

				indexVer1 = arrayDown[0];
				indexVer2 = arrayUp[1];
				temp = layer[a].numOfCoor;
				//x2
				layer[a].interMat[temp][0] = (zPlane-poly[p].vertex[indexVer2].z)*(poly[p].vertex[indexVer2].x-poly[p].vertex[indexVer1].x)/(poly[p].vertex[indexVer2].z-poly[p].vertex[indexVer1].z)+poly[p].vertex[indexVer2].x+xOffset;
				//y2
				layer[a].interMat[temp][1] = (zPlane-poly[p].vertex[indexVer2].z)*(poly[p].vertex[indexVer2].y-poly[p].vertex[indexVer1].y)/(poly[p].vertex[indexVer2].z-poly[p].vertex[indexVer1].z)+poly[p].vertex[indexVer2].y+yOffset;
				layer[a].interMat[temp][2] = layer[a].numOfSeg;
				layer[a].interMat[temp][3] = 2;
				layer[a].numOfCoor++;
				layer[a].numOfSeg++;
			}

		}
	}
	else if(countZequal==1)
	{
		if(countZDown == countZup)//one above one beneath
		{
			//intersection with 1 facets and 1 vertix
			indexVer = arrayEqual[0];
			temp = layer[a].numOfCoor;
			layer[a].interMat[temp][0] = poly[p].vertex[indexVer].x + xOffset;   //x1
			layer[a].interMat[temp][1] = poly[p].vertex[indexVer].y + yOffset;   //y1
			layer[a].interMat[temp][2] = layer[a].numOfSeg;
			layer[a].interMat[temp][3] = 1;
			layer[a].numOfCoor++;

			indexVer = arrayUp[0];
			indexVer2 = arrayDown[0];
			temp = layer[a].numOfCoor;
			//x2
			layer[a].interMat[temp][0] = (zPlane-poly[p].vertex[indexVer].z)*(poly[p].vertex[indexVer].x-poly[p].vertex[indexVer2].x)/(poly[p].vertex[indexVer].z-poly[p].vertex[indexVer2].z)+poly[p].vertex[indexVer].x+xOffset;
			//y2
			layer[a].interMat[temp][1] = (zPlane-poly[p].vertex[indexVer].z)*(poly[p].vertex[indexVer].y-poly[p].vertex[indexVer2].y)/(poly[p].vertex[indexVer].z-poly[p].vertex[indexVer2].z)+poly[p].vertex[indexVer].y+yOffset;
			layer[a].interMat[temp][2] = layer[a].numOfSeg;
			layer[a].interMat[temp][3] = 2;
			layer[a].numOfCoor++;
			layer[a].numOfSeg++;
		}
		else	 //intersection with 1 vertix (and no facet)	  
		{

			indexVer = arrayEqual[0];
			temp = layer[a].numOfCoor;
			layer[a].x_arr[temp] = poly[p].vertex[indexVer].x+xOffset;
			layer[a].y_arr[temp] = poly[p].vertex[indexVer].y+yOffset;
			layer[a].numOfCoor++;
		}
	}
	else if(countZequal>=2)
	{
		//intersection with 2 vertices (1 facet merged)

		indexVer = arrayEqual[0];
		temp = layer[a].numOfCoor;
		layer[a].interMat[temp][0] = poly[p].vertex[indexVer].x+xOffset;   //x1
		layer[a].interMat[temp][1] = poly[p].vertex[indexVer].y+yOffset;   //y1
		layer[a].interMat[temp][2] = layer[a].numOfSeg;
		layer[a].interMat[temp][3] = 1;
		layer[a].numOfCoor++;

		indexVer = arrayEqual[1];
		temp = layer[a].numOfCoor;
		layer[a].interMat[temp][0] = poly[p].vertex[indexVer].x+xOffset;   //x2
		layer[a].interMat[temp][1] = poly[p].vertex[indexVer].y+yOffset;   //y2
		layer[a].interMat[temp][2] = layer[a].numOfSeg;
		layer[a].interMat[temp][3] = 2;
		layer[a].numOfCoor++;
		layer[a].numOfSeg++;
	}

}

//---------------------------------------------------------------------------------------------------------//
//---------------------------------- Loading the model & getting the data ---------------------------------//
//---------------------------------------------------------------------------------------------------------//
int CVICALLBACK importStlFunction (int panel, int control, int event,
								   void *callbackData, int eventData1, int eventData2)
{
	int filenameStat = 0, n=0;
	char modelName[50];

	switch (event)
	{
		case EVENT_COMMIT:

			modelH = 0;
			xMax = 0;
			xMin = 10000;
			yMax = 0;
			yMin = 10000;
			zMin = 1000.0;
			lastScaleFactor = 1;

			SetCtrlAttribute (panelHandle, PANEL_TIMER1, ATTR_ENABLED, 0);

			filenameStat = FileSelectPopup ("3D Models", "*.stl", "", "Select File", VAL_LOAD_BUTTON, 0, 1, 1, 0, filename);

			if(filenameStat==1)
			{
				SetCtrlAttribute (panelHandle, PANEL_NUMERIC_THICKNESS, ATTR_CTRL_MODE, VAL_HOT);
				SetCtrlAttribute (panelHandle, PANEL_BUTTON_SIMULATE, ATTR_CTRL_MODE, VAL_INDICATOR);
				SetCtrlAttribute (panelHandle, PANEL_BUTTON_PRINT, ATTR_CTRL_MODE, VAL_INDICATOR);
				SetCtrlAttribute (panelHandle, PANEL_NUMERIC_SCALE, ATTR_CTRL_MODE, VAL_INDICATOR);
				SetCtrlAttribute (panelHandle, PANEL_NUMERIC_SLIDER, ATTR_VISIBLE, 0);
				SetCtrlAttribute (panelHandle, PANEL_BUTTON_GCODE, ATTR_CTRL_MODE, VAL_INDICATOR);
				SetCtrlAttribute (panelHandle, PANEL_BUTTON_INTERSECTIO, ATTR_CTRL_MODE, VAL_INDICATOR);
				SetCtrlAttribute (panelHandle, PANEL_TEXTMSG_PRINT, ATTR_DIMMED, 1);
				DeleteGraphPlot (panelHandle, PANEL_GRAPH, -1, VAL_IMMEDIATE_DRAW);
				fp = fopen (filename, "r");
				fgets (fileLine, 199, fp);

				while(fileLine[n+6]!=0)
				{
					modelName[n]=fileLine[n+6];
					n++;
				}
				SetCtrlVal (panelHandle, PANEL_STRING_MODEL, modelName);

				findPolyNumFunc();
				SetCtrlVal (panelHandle, PANEL_NUMERIC_POLY, polyCounter);
				SetCtrlVal (panelHandle, PANEL_NUMERIC_VERTICES, polyCounter*3);
				poly = (polygon*)calloc(polyCounter,sizeof(polygon));	//allocating an array for all polygons

				modelHeightFunc();
				if(modelH>220)
				{
					MessagePopup ("Note!", "Model is higher than 220mm. Please choose another model.");
					break;
				}
				SetCtrlVal (panelHandle, PANEL_NUMERIC_HEIGHT, modelH);

				GetCtrlVal (panelHandle, PANEL_NUMERIC_THICKNESS, &layerT);
				totalLayers = (modelH/layerT)+1;							  //+1 to fix error	   
				SetCtrlVal (panelHandle, PANEL_NUMERIC_LAYER, totalLayers);

				SetCtrlVal (panelHandle, PANEL_NUMERIC_WIDTH, modelWidth);
				SetCtrlVal (panelHandle, PANEL_NUMERIC_LENGTH, modelLength);
				SetCtrlVal (panelHandle, PANEL_NUMERIC_HEIGHT, modelH);

				//calculating the coordinates of center of the model
				xMid = ((xMax-xMin)/2)+xMin;
				yMid = ((yMax-yMin)/2)+yMin;

				//calculating the distance from center of the model to the center of graph/printer bed
				//and this will be used for all the calculations for centerind the model on the graph
				xOffset = (bedwidth/2)-xMid;
				yOffset = (bedlength/2)-yMid;

				SetCtrlAttribute (panelHandle, PANEL_BUTTON_SLICE, ATTR_CTRL_MODE, VAL_HOT);

				SetCtrlVal (panelHandle, PANEL_NUMERIC_SCALE, 1.00);
				SetCtrlAttribute (panelHandle, PANEL_TEXTMSG_SLICING, ATTR_DIMMED, 0);
			}

			break;
	}
	return 0;
}
//---------------------------------------------------------------------------------------------------------//
//----------------For case when the user change the layer thickness after loading the model----------------//
//---------------------------------------------------------------------------------------------------------//
int CVICALLBACK layersThicknessFunc (int panel, int control, int event,
									 void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			GetCtrlVal (panelHandle, PANEL_NUMERIC_THICKNESS, &layerT);
			totalLayers = modelH/layerT;
			SetCtrlVal (panelHandle, PANEL_NUMERIC_LAYER, totalLayers);

			break;
	}
	return 0;
}
//---------------------------------------------------------------------------------------------------------//
//----------------------------------Pressing the Slice button---------------------------------------------//
//---------------------------------------------------------------------------------------------------------//
int CVICALLBACK sliceFunction (int panel, int control, int event,
							   void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:

			DeleteGraphPlot (panelHandle, PANEL_GRAPH, -1, VAL_IMMEDIATE_DRAW);
			SetCtrlAttribute (panelHandle, PANEL_BUTTON_SLICE, ATTR_CTRL_MODE, VAL_INDICATOR);
			SetCtrlAttribute (panelHandle, PANEL_NUMERIC_THICKNESS, ATTR_CTRL_MODE, VAL_INDICATOR);

			layer = (layers*)calloc(totalLayers, sizeof(layers));

			for(int a=0; a<totalLayers; a++)//running on each layer
			{
				layer[a].numOfCoor = 0;
				layer[a].numOfSeg = 1;

				for(int p=0; p<polyCounter; p++)//running on all polygons
				{
					intersectionFunc(a,p);
				}

				sortFunction(a);
			}

			initSlider();
			PlotXY (panelHandle, PANEL_GRAPH, layer[0].x_arr, layer[0].y_arr, layer[0].numOfSeg, VAL_FLOAT, VAL_FLOAT, VAL_CONNECTED_POINTS, VAL_NO_POINT, VAL_SOLID, 1, VAL_WHITE);
			SetCtrlAttribute (panelHandle, PANEL_BUTTON_INTERSECTIO, ATTR_CTRL_MODE, VAL_HOT);
			SetCtrlAttribute (panelHandle, PANEL_NUMERIC_SCALE, ATTR_CTRL_MODE, VAL_HOT);
			SetCtrlAttribute (panelHandle, PANEL_BUTTON_GCODE, ATTR_CTRL_MODE, VAL_HOT);
			SetCtrlAttribute (panelHandle, PANEL_BUTTON_SIMULATE, ATTR_CTRL_MODE, VAL_HOT);
			SetCtrlAttribute (panelHandle, PANEL_BUTTON_PRINT, ATTR_CTRL_MODE, VAL_HOT);
			SetCtrlAttribute (panelHandle, PANEL_TEXTMSG_PRINT, ATTR_DIMMED, 0);

			break;
	}
	return 0;
}


//---------------------------------------------------------------------------------------------------------//
//------------ plotting the contour of the model according to the chosen layer of the slider --------------//
//---------------------------------------------------------------------------------------------------------//
int CVICALLBACK sliderFunction (int panel, int control, int event,
								void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			DeleteGraphPlot (panelHandle, PANEL_GRAPH, -1, VAL_IMMEDIATE_DRAW);
			GetCtrlAttribute (panelHandle, PANEL_NUMERIC_SLIDER, ATTR_CTRL_VAL, &currentLayer);
			if(layer[currentLayer-1].numOfSeg != 0)
			{
				PlotXY (panelHandle, PANEL_GRAPH, layer[currentLayer-1].x_arr, layer[currentLayer-1].y_arr, layer[currentLayer-1].numOfSeg, VAL_FLOAT, VAL_FLOAT, VAL_CONNECTED_POINTS, VAL_NO_POINT, VAL_SOLID, 1, VAL_WHITE);
			}

			break;
	}
	return 0;
}
//---------------------------------------------------------------------------------------------------------		
int CVICALLBACK exportIntersection (int panel, int control, int event,
									void *callbackData, int eventData1, int eventData2)
{
	FILE *fp1;

	switch (event)
	{
		case EVENT_COMMIT:

			fp1 = fopen ("Contour Points.txt", "w");
			fprintf (fp1,"       X               Y               Z\n");
			
			for(int a=0; a<totalLayers; a++)
			{
				for(int i=0; i<layer[a].numOfSeg; i++)
				{
					fprintf (fp1,"%f   %f   %f\n",layer[a].x_arr[i],layer[a].y_arr[i],layerT*(a+1));
				}
			}

			fclose (fp1);

			break;
	}
	return 0;
}

//---------------------------------------------------------------------------------------------------------
int CVICALLBACK openSimulationFunc (int panel, int control, int event,
									void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			SetCtrlAttribute (panelHandle, PANEL_TIMER1, ATTR_ENABLED, 0);
			GetCtrlAttribute (panelHandle, PANEL_NUMERIC_SLIDER, ATTR_CTRL_VAL, &currentLayer);
			PlotXY (panelHandle2, PANEL_SIMU_GRAPH_SIMULATION, layer[currentLayer-1].x_arr, layer[currentLayer-1].y_arr, layer[currentLayer-1].numOfSeg,
					VAL_FLOAT, VAL_FLOAT, VAL_THIN_LINE, VAL_NO_POINT, VAL_SOLID, 1, VAL_RED);
			PlotPoint (panelHandle2, PANEL_SIMU_GRAPH_SIMULATION, layer[currentLayer-1].x_arr[0], layer[currentLayer-1].y_arr[0], VAL_SOLID_CIRCLE, VAL_DK_BLUE);
			counter = 0;
			top = ((180-layer[currentLayer-1].y_arr[counter])*3.5556)-138;
			left = ((layer[currentLayer-1].x_arr[counter])*3.5556)-56;
			SetCtrlAttribute (panelHandle2, PANEL_SIMU_PICTURE, ATTR_TOP, top);
			SetCtrlAttribute (panelHandle2, PANEL_SIMU_PICTURE, ATTR_LEFT, left);
			DisplayPanel (panelHandle2);
			counter++;
			break;
	}
	return 0;
}
//---------------------------------------------------------------------------------------------------------
int CVICALLBACK timerFunction (int panel, int control, int event,
							   void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_TIMER_TICK:

			if(counter<layer[currentLayer-1].numOfSeg)
			{
				//56,138 is the position of the tip of the nozzle in the picture, at pixles
				top = ((180-layer[currentLayer-1].y_arr[counter])*3.5556)-138;
				left = ((layer[currentLayer-1].x_arr[counter])*3.5556)-56;

				SetCtrlAttribute (panelHandle2, PANEL_SIMU_PICTURE, ATTR_TOP, top);
				SetCtrlAttribute (panelHandle2, PANEL_SIMU_PICTURE, ATTR_LEFT, left);
				counter++;
			}
			else
			{
				SetCtrlAttribute (panelHandle2, PANEL_SIMU_TIMER, ATTR_ENABLED, 0);
				SetCtrlAttribute (panelHandle2, PANEL_SIMU_BUTTON_DRAW, ATTR_CTRL_MODE, VAL_HOT);
				counter = 0;
			}

			break;
	}
	return 0;
}
//---------------------------------------------------------------------------------------------------------
int CVICALLBACK runSimulationFunc (int panel, int control, int event,
								   void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			SetCtrlAttribute (panelHandle2, PANEL_SIMU_BUTTON_DRAW, ATTR_CTRL_MODE, VAL_INDICATOR);
			SetCtrlAttribute (panelHandle2, PANEL_SIMU_TIMER, ATTR_ENABLED, 1);

			break;
	}
	return 0;
}
//---------------------------------------------------------------------------------------------------------
int CVICALLBACK stopFunc (int panel, int control, int event,
						  void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			SetCtrlAttribute (panelHandle2, PANEL_SIMU_TIMER, ATTR_ENABLED, 0);
			SetCtrlAttribute (panelHandle2, PANEL_SIMU_BUTTON_DRAW, ATTR_CTRL_MODE, VAL_HOT);
			break;
	}
	return 0;
}
//---------------------------------------------------------------------------------------------------------//
//-----------------------------------------Export the G-code-----------------------------------------------//
//---------------------------------------------------------------------------------------------------------//
int CVICALLBACK gcodeFunc (int panel, int control, int event,
						   void *callbackData, int eventData1, int eventData2)
{
	FILE *fp4;
	int filenameStat=0;
	char filename2[260];
	float filamentArea, filamentDiameter=1.75, extrusionVolume, exFactor, nozzle_diameter=0.4, distance,extrude=0, errorF=1.4;

	switch (event)
	{
		case EVENT_COMMIT:

			if((modelWidth*lastScaleFactor) > 180 || (modelLength*lastScaleFactor) > 180)
			{
				MessagePopup ("Note! ", "The model is too big and cannot be print. Please reduce model size.");
				break;
			}
			MessagePopup ("Please Note!", "The program generates G-code for contours only and it doesn't include infill.");
			filenameStat = FileSelectPopup ("", "*.gcode", "", "Save File", VAL_SAVE_BUTTON, 0, 0, 1, 0, filename2);
			if(filenameStat>=1)
			{
				fp4 = fopen (filename2, "w");	

				//calculating flow speed factor
				filamentArea = 3.14159f * filamentDiameter*filamentDiameter / 4;	//section area
				extrusionVolume = nozzle_diameter * layerT;
				exFactor = extrusionVolume*errorF / filamentArea;

				GetCtrlVal (panelHandle, PANEL_NUMERIC_BEDTEMP, &bedTemp);
				GetCtrlVal (panelHandle, PANEL_NUMERIC_NOZZTEMP, &nozzleTemp);
				GetCtrlVal (panelHandle, PANEL_NUMERIC_PSPEED, &printSpeed);

				//Start G-code
				fprintf (fp4,"M190 S%d\nM109 S%d\nG21\nG90\nM82\nM107\nG28 X0 Y0\nG28 Z0\nG1 Z15.0 F3000\nG92 E0\nG1 F200 E3\nG92 E0\nG1 F3000\nM117 Printing...\n\n", bedTemp,nozzleTemp);

				//main G-code
				for(int a=0; a<totalLayers; a++)
				{
					fprintf (fp4,";LAYER:%d\nM106 S127\n",a);   //new layer, set fan speed
					fprintf (fp4,"G0 F3000 X%f Y%f Z%f\n", layer[a].x_arr[0], layer[a].y_arr[0],layerT*(a+1)); //moving to first point in this layer
					
					if(a==0 || a==1)	//first and second layer slower
						fprintf (fp4,"G1 F300\n");
					else
						fprintf (fp4,"G1 F%d\n",printSpeed*60);

					//outer contour printing
					for (int i=1; i<layer[a].numOfSeg; i++)
					{

						distance = sqrt(pow(layer[a].x_arr[i]-layer[a].x_arr[i-1],2) + pow(layer[a].y_arr[i]-layer[a].y_arr[i-1],2));
						
						if(a==0 || a==1)								 //flow speed.  (1.2 for start error)
							extrude = (exFactor*distance)*1.2 + extrude;
						else
							extrude = (exFactor*distance) + extrude;	//flow speed

						fprintf (fp4,"G1 X%f Y%f E%f\n", layer[a].x_arr[i], layer[a].y_arr[i], extrude);
					}

				}

				//End G-code
				fprintf(fp4,"\nM104 S0\nM140 S0\nG91\nG1 E-1 F300\nG1 Z+1.2 E-5 X-20 Y-20 F3000\nG28 X0 Y0\nM84\nG90\nM81");

				MessagePopup ("Note!", "Please close the program before copy the G-code to an external drive.");
			}

			break;
	}
	return 0;
}
//---------------------------------------------------------------------------------------------------------//
//-----------------------------------------Scaling the model-----------------------------------------------//
//---------------------------------------------------------------------------------------------------------//
int CVICALLBACK scaleFunction (int panel, int control, int event,
							   void *callbackData, int eventData1, int eventData2)
{
	double temp;

	switch (event)
	{
		case EVENT_COMMIT:

			DeleteGraphPlot (panelHandle, PANEL_GRAPH, -1, VAL_IMMEDIATE_DRAW);
			GetCtrlAttribute (panelHandle, PANEL_NUMERIC_SLIDER, ATTR_CTRL_VAL, &currentLayer);
			GetCtrlVal (panelHandle, PANEL_NUMERIC_SCALE, &scaleFactor);
			temp = scaleFactor;
			scaleFactor = scaleFactor/lastScaleFactor;

			for(int a=0; a<totalLayers; a++)
			{
				for(int i=0; i<layer[a].numOfSeg; i++)
				{
					if(layer[a].x_arr[i] > 90)						//(x,y) = (90,90) it's the point of middle of the bed (for offsetting)
					{
						layer[a].x_arr[i]= ((layer[a].x_arr[i]-90)*scaleFactor) + 90;

						if(layer[a].y_arr[i] > 90)
						{
							layer[a].y_arr[i]= ((layer[a].y_arr[i]-90)*scaleFactor) + 90;
						}
						else if(layer[a].y_arr[i] < 90)
						{
							layer[a].y_arr[i]= 90 - ((90-layer[a].y_arr[i])*scaleFactor);
						}
					}
					else if(layer[a].x_arr[i] < 90)
					{
						layer[a].x_arr[i]= 90 - ((90-layer[a].x_arr[i])*scaleFactor);

						if(layer[a].y_arr[i] > 90)
						{
							layer[a].y_arr[i]= ((layer[a].y_arr[i]-90)*scaleFactor) + 90;
						}
						else if(layer[a].y_arr[i] < 90)
						{
							layer[a].y_arr[i]= 90 - ((90-layer[a].y_arr[i])*scaleFactor);
						}
					}
					else
					{
						if(layer[a].y_arr[i] > 90)
						{
							layer[a].y_arr[i]= ((layer[a].y_arr[i]-90)*scaleFactor) + 90;
						}
						else if(layer[a].y_arr[i] < 90)
						{
							layer[a].y_arr[i]= 90 - ((90-layer[a].y_arr[i])*scaleFactor);
						}
					}

				}
			}


			if(layer[currentLayer-1].numOfSeg != 0)
			{
				PlotXY (panelHandle, PANEL_GRAPH, layer[currentLayer-1].x_arr, layer[currentLayer-1].y_arr, layer[currentLayer-1].numOfSeg, VAL_FLOAT, VAL_FLOAT, VAL_CONNECTED_POINTS, VAL_NO_POINT, VAL_SOLID, 1, VAL_WHITE);
			}

			lastScaleFactor = temp;

			SetCtrlVal (panelHandle, PANEL_NUMERIC_WIDTH, modelWidth*lastScaleFactor);
			SetCtrlVal (panelHandle, PANEL_NUMERIC_LENGTH, modelLength*lastScaleFactor);

			break;
	}
	return 0;
}
//----------------------------------------------------------------------------------------------------------------------------
void CVICALLBACK exitFunction (int menuBar, int menuItem, void *callbackData,
							   int panel)
{
	QuitUserInterface (0);
}

//----------------------------------------------------------------------------------------------------------------------------
void CVICALLBACK aboutFunc (int menuBar, int menuItem, void *callbackData,
							int panel)
{
	DisplayPanel (panelHandle3);
}
//----------------------------------------------------------------------------------------------------------------------------
int CVICALLBACK printFunc (int panel, int control, int event,
						   void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			SetCtrlAttribute (panelHandle, PANEL_NUMERIC_SCALE, ATTR_CTRL_MODE, VAL_INDICATOR);
			SetCtrlAttribute (panelHandle, PANEL_TIMER1, ATTR_ENABLED, 1);
			counter = 0;
			SetCtrlAttribute (panelHandle, PANEL_BUTTON_SIMULATE, ATTR_CTRL_MODE, VAL_INDICATOR);
			break;
	}
	return 0;
}
//----------------------------------------------------------------------------------------------------------------------------
int CVICALLBACK tickFunction (int panel, int control, int event,
							  void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_TIMER_TICK:
			if(counter < totalLayers)
			{
				DeleteGraphPlot (panelHandle, PANEL_GRAPH, -1, VAL_IMMEDIATE_DRAW);
				PlotXY (panelHandle, PANEL_GRAPH, layer[counter].x_arr, layer[counter].y_arr, layer[counter].numOfSeg, VAL_FLOAT, VAL_FLOAT, VAL_CONNECTED_POINTS, VAL_NO_POINT, VAL_SOLID, 1, VAL_WHITE);
				SetCtrlAttribute (panelHandle, PANEL_NUMERIC_SLIDER, ATTR_CTRL_MODE, VAL_INDICATOR);
				SetCtrlAttribute (panelHandle, PANEL_NUMERIC_SLIDER, ATTR_CTRL_VAL, 1+counter);
				counter++;
			}
			else
			{
				SetCtrlAttribute (panelHandle, PANEL_TIMER1, ATTR_ENABLED, 0);
				SetCtrlAttribute (panelHandle, PANEL_NUMERIC_SLIDER, ATTR_CTRL_MODE, VAL_HOT);
				SetCtrlAttribute (panelHandle, PANEL_NUMERIC_SCALE, ATTR_CTRL_MODE, VAL_HOT);
				SetCtrlAttribute (panelHandle, PANEL_BUTTON_SIMULATE, ATTR_CTRL_MODE, VAL_HOT);
			}


			break;
	}
	return 0;
}
//----------------------------------------------------------------------------------------------------------------------------
void CVICALLBACK clearPlatformFunc (int menuBar, int menuItem, void *callbackData,
									int panel)
{
	DeleteGraphPlot (panelHandle, PANEL_GRAPH, -1, VAL_IMMEDIATE_DRAW);
	SetCtrlVal (panelHandle, PANEL_STRING_MODEL, "");
	SetCtrlAttribute (panelHandle, PANEL_NUMERIC_SLIDER, ATTR_VISIBLE, 0);
	SetCtrlVal (panelHandle, PANEL_NUMERIC_WIDTH, 0.00);
	SetCtrlVal (panelHandle, PANEL_NUMERIC_LENGTH, 0.00);
	SetCtrlVal (panelHandle, PANEL_NUMERIC_HEIGHT, 0.00);
	SetCtrlVal (panelHandle, PANEL_NUMERIC_POLY, 0);
	SetCtrlVal (panelHandle, PANEL_NUMERIC_VERTICES, 0);
	SetCtrlVal (panelHandle, PANEL_NUMERIC_LAYER, 0);
	HidePanel(panelHandle2);
	SetCtrlAttribute (panelHandle, PANEL_TIMER1, ATTR_ENABLED, 0);
	SetCtrlAttribute (panelHandle, PANEL_BUTTON_SIMULATE, ATTR_CTRL_MODE, VAL_INDICATOR);
	SetCtrlAttribute (panelHandle, PANEL_BUTTON_PRINT, ATTR_CTRL_MODE, VAL_INDICATOR);
	SetCtrlAttribute (panelHandle, PANEL_NUMERIC_SCALE, ATTR_CTRL_MODE, VAL_INDICATOR);
	SetCtrlAttribute (panelHandle, PANEL_BUTTON_GCODE, ATTR_CTRL_MODE, VAL_INDICATOR);
	SetCtrlAttribute (panelHandle, PANEL_BUTTON_INTERSECTIO, ATTR_CTRL_MODE, VAL_INDICATOR);
	SetCtrlAttribute (panelHandle, PANEL_BUTTON_SLICE, ATTR_CTRL_MODE, VAL_INDICATOR);
	SetCtrlAttribute (panelHandle, PANEL_TEXTMSG_SLICING, ATTR_DIMMED, 1);
	SetCtrlAttribute (panelHandle, PANEL_TEXTMSG_PRINT, ATTR_DIMMED, 1);
}
//----------------------------------------------------------------------------------------------------------------------------
void CVICALLBACK pdfFunc (int menuBar, int menuItem, void *callbackData,
						  int panel)
{
	OpenDocumentInDefaultViewer ("Info.pdf", VAL_NO_ZOOM);
}

//----------------------------------------------------------------------------------------------------------------------------
void CVICALLBACK demoFunc (int menuBar, int menuItem, void *callbackData,
						   int panel)
{
	OpenDocumentInDefaultViewer ("Demo.mp4", VAL_NO_ZOOM);
}

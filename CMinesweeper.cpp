#include "CMinesweeper.h"

//-----------------------------------constructor-------------------------
//
//-----------------------------------------------------------------------
CMinesweeper::CMinesweeper():
m_dRotation(0),
	m_lTrack(0),
	//m_rTrack(0),
	m_dFitness(0),
	m_dScale(CParams::iSweeperScale),
	m_bCollided(false)


{
	m_Color = CreatePen(PS_SOLID, 1, RGB(rand() % 205 + 50, rand() % 205 + 50, rand() % 205 + 50));
	//create a static start position
	m_vPosition = SVector2D(5, 1);

	//create the sensors
//	CreateSensors(m_Sensors, CParams::iNumSensors, CParams::dSensorRange); 

	//initialize its memory
	m_MemoryMap.Init(CParams::WindowWidth,
		CParams::WindowHeight);

}


//-------------------------------- CreateSensors ------------------------
//
//	This function returns a vector of points which make up the segments of
//  the sweepers sensors.
//------------------------------------------------------------------------
void CMinesweeper::CreateSensors(vector<SPoint> &sensors,
	int            NumSensors,
	double         range)
{
	//make sure vector of sensors is empty before proceeding
	sensors.clear();

	//double SegmentAngle = CParams::dPi / (NumSensors-1);

	////going clockwise from 90deg left of position calculate the fan of
	////points radiating out (not including the origin)
	//for (int i=0; i<CParams::iNumSensors; i++)
	//{
	//	//calculate vertex position
	//	SPoint point;

	//	point.x = -sin(i * SegmentAngle - CParams::dHalfPi) * range;

	//	point.y = cos(i * SegmentAngle - CParams::dHalfPi) * range;

	//	sensors.push_back(point);

	//}//next segment
	SPoint point;
	point.x = -sin((double)-0.5235) * range;
	point.y = cos((double)-0.5235) * range;

	sensors.push_back(point);

	SPoint point2;
	point2.x = -sin((double)0.5235) * range;
	point2.y = cos((double)0.5235) * range;

	sensors.push_back(point2);
}
//-----------------------------Reset()------------------------------------
//
//	Resets the sweepers position, fitness and rotation
//
//------------------------------------------------------------------------
void CMinesweeper::Reset()
{
	//reset the sweepers positions
	m_vPosition = SVector2D(5, 1);

	//and the fitness
	m_dFitness = 0;

	//and the rotation
	m_dRotation = 0;

	m_bCollided = false;

	m_bReverse = false;

	//reset its memory
	m_MemoryMap.Reset();

	m_bActive = true;
}

void CMinesweeper::SetReverse(bool reverse) 
{
	m_bReverse = reverse;
}

int CMinesweeper::ResetTrial(int generation) 
{
	double reward = m_MemoryMap.CheckReward(m_vPosition.x, m_vPosition.y, m_bReverse);
	m_dFitness += generation > 0 ? reward : 0;
	//m_dFitness += generation > 0 ? m_MemoryMap.TMazeReward(m_bReverse) : 0;
	m_vPosition = SVector2D(5, 1);
	m_dRotation = 0;

	//reset its memory
	m_MemoryMap.Reset();

	// Flush neural network
	//this will store all the inputs for the NN
	vector<double> inputs;	

	//grab sensor readings
	//	TestSensors(objects);


	//input sensors into net
	for (int sr=0; sr<m_vecdSensors.size(); ++sr)
	{
		inputs.push_back(m_vecdSensors[sr]);

	//	inputs.push_back(m_vecFeelers[sr]);
	}

//	inputs.push_back(m_bCollided);
	// Reward
	inputs.push_back(0);
	m_bCollided = false;
	// Turning point
	inputs.push_back(0);
	//update the brain and get feedback
	vector<double> output = m_pItsBrain->Update(inputs, CNeuralNet::snapshot);

	m_bActive = true;
	if (reward < 0.01) {
		return 0;
	}
	if (m_bReverse) {
		if (reward > 0.9) {
			return -1;
		}
		else {
			return 1;
		}
	} else {
		if (reward > 0.9) {
			return 1;
		}
		else {
			return -1;
		}
	}
}

//------------------------- RenderMemory ---------------------------------
//
//------------------------------------------------------------------------
void CMinesweeper::Render(HDC surface)
{
	//render the memory
	m_MemoryMap.Render(surface);

	string s = itos(m_MemoryMap.NumCellsVisited());
	s = "Num Cells Visited: " + s;
	TextOut(surface, 220,0,s.c_str(), s.size());

}

//---------------------WorldTransform--------------------------------
//
//	sets up a translation matrix for the sweeper according to its
//  scale, rotation and position. Returns the transformed vertices.
//-------------------------------------------------------------------
void CMinesweeper::WorldTransform(vector<SPoint> &sweeper, double scale)
{
	//create the world transformation matrix
	C2DMatrix matTransform;

	//scale
	matTransform.Scale(scale, scale);

	//rotate
	matTransform.Rotate(m_dRotation);

	//and translate
	matTransform.Translate(m_vPosition.x, m_vPosition.y);

	//now transform the ships vertices
	matTransform.TransformSPoints(sweeper);
}

//-------------------------------Update()--------------------------------
//
//	First we take sensor readings and feed these into the sweepers brain.
//
//	The inputs are:
//	
//  The readings from the minesweepers sensors
//
//	We receive two outputs from the brain.. lTrack & rTrack.
//	So given a force for each track we calculate the resultant rotation 
//	and acceleration and apply to current velocity vector.
//
//-----------------------------------------------------------------------
bool CMinesweeper::Update(vector<SPoint> &objects)
{
	if (m_bActive) {	
		//this will store all the inputs for the NN
		vector<double> inputs;	

		//grab sensor readings
		TestSensors(objects);


		//input sensors into net
		for (int sr=0; sr<m_vecdSensors.size(); ++sr)
		{
			inputs.push_back(m_vecdSensors[sr]);

		//	inputs.push_back(m_vecFeelers[sr]); // No need for feelers
		}

	//	inputs.push_back(m_bCollided);

		double reward = m_MemoryMap.CheckReward(m_vPosition.x, m_vPosition.y, m_bReverse);

		

		inputs.push_back(reward);

		double turningPoint = m_MemoryMap.CheckTurningPoint(m_vPosition.x, m_vPosition.y);

		inputs.push_back(turningPoint);

		//update the brain and get feedback
		vector<double> output = m_pItsBrain->Update(inputs, CNeuralNet::active);

		//make sure there were no errors in calculating the 
		//output
		if (output.size() < CParams::iNumOutputs) 
		{
			return false;
		}

		//assign the outputs to the sweepers left & right tracks
	//	m_lTrack = output[0] * 2 - 1;
		//m_rTrack = output[1];
		m_lTrack = output[0] * 2 - 1;
		//calculate steering forces
		//double RotForce = m_lTrack - m_rTrack;


		//clamp rotation
		//Clamp(RotForce, -CParams::dMaxTurnRate, CParams::dMaxTurnRate);

		//if(m_lTrack < -0.3) m_dRotation = 0;//3.14159265358979;//3.14159265358979f * (3.0 / 2.0);
		//else if(m_lTrack > 0.3) m_dRotation = 3.1415926358979f * 0.5;
		//else m_dRotation = 3.1415926358979f * 1.5;

		double rot = 0;

		if (m_lTrack < -0.3) {
			rot = 3.14159265358979 * 0.5;
		} 
		if (m_lTrack > 0.3) {
			rot = 3.14159265358979 * 1.5;
		} 

		m_dRotation += rot;

		//update Look At 
		m_vLookAt.x = -sin(m_dRotation);
		m_vLookAt.y = cos(m_dRotation);

		if (m_vLookAt.x > 0.9) {
			m_vLookAt.x = 1;
		} else if (m_vLookAt.x < - 0.9) {
			m_vLookAt.x = -1;
		} else {
			m_vLookAt.x = 0;
		}

		if (m_vLookAt.y > 0.9) {
			m_vLookAt.y = 1;
		} else if (m_vLookAt.y < - 0.9) {
			m_vLookAt.y = -1;
		} else {
			m_vLookAt.y = 0;
		}
		
		

		//if the sweepers haven't collided with an obstacle
		//update their position
		if (!m_bCollided)
		{
			//m_dSpeed = 1;// + m_rTrack;

			//m_dSpeed *= 2;

			//update position
			m_vPosition += (m_vLookAt);

			//test range of x,y values - because of 'cheap' collision detection
			//this can go into error when using < 4 sensors
			//TestRange();
		}

		//update the memory map
	//	m_MemoryMap.Update(m_vPosition.x, m_vPosition.y);

		m_bActive = reward < 0.05;

		return true;
	}
	return true;
}


//----------------------- TestSensors ------------------------------------
//
//  This function checks for any intersections between the sweeper's 
//  sensors and the objects in its environment
//------------------------------------------------------------------------
void CMinesweeper::TestSensors(vector<SPoint> &objects)
{
	//m_bCollided = false;  

	////first we transform the sensors into world coordinates
	//m_tranSensors = m_Sensors;

	//WorldTransform(m_tranSensors, 1);  //scale is 1

	////flush the sensors
	//m_vecdSensors.clear();
	//m_vecFeelers.clear();

	////now to check each sensor against the objects in the world
	//for (int sr=0; sr<m_tranSensors.size(); ++sr)
	//{
	//	bool bHit = false;

	//	double dist = 0;

	//	for (int seg=0; seg<objects.size(); seg+=2)
	//	{
	//		if (LineIntersection2D(SPoint(m_vPosition.x, m_vPosition.y),
	//			m_tranSensors[sr],
	//			objects[seg],
	//			objects[seg+1],
	//			dist))
	//		{
	//			bHit = true;

	//			break;        
	//		}
	//	}

	//	if (bHit)
	//	{
	//		m_vecdSensors.push_back(dist);

	//		//implement very simple collision detection
	//		if (dist < CParams::dCollisionDist)
	//		{
	//			m_bCollided = true;
	//		}
	//	}

	//	else
	//	{
	//		m_vecdSensors.push_back(-1);
	//	} 

	//	 

	//}//next sensor

	if (!m_bCollided) {
		if (m_vPosition.y < 0) {
			m_bCollided = true;
			return;
		}
		if (m_vPosition.y > 6) {
			m_bCollided = true;
			return;
		}
		if (m_vPosition.y < 4) {
			if (m_vPosition.x > 6) {
				m_bCollided = true;
				return;
			}
			if (m_vPosition.x < 4) {
				m_bCollided = true;
				return;
			}
		}
	}
}

//-------------------------------- TestRange -----------------------------
//
//------------------------------------------------------------------------
void CMinesweeper::TestRange()
{
	if (m_vPosition.x < 0)
	{
		m_vPosition.x = 0;
	}

	if (m_vPosition.x > CParams::WindowWidth)
	{
		m_vPosition.x = CParams::WindowWidth;
	}

	if (m_vPosition.y < 0)
	{
		m_vPosition.y = 0;
	}

	if (m_vPosition.y > CParams::WindowHeight)
	{
		m_vPosition.y = CParams::WindowHeight;
	}
}


//------------------------- EndOfRunCalculations() -----------------------
//
//------------------------------------------------------------------------
void CMinesweeper::EndOfRunCalculations()
{
	//m_dFitness += m_MemoryMap.NumCellsVisited();
	//m_dFitness += m_MemoryMap.TMazeReward();

	// Fitness is the average reward/fitness over number of trials
	m_dFitness = m_dFitness / CParams::iNumTrials;
}






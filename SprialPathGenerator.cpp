#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <cassert>

using namespace std;

typedef unsigned int UInt;

UInt m_uiSearchRange = 0;
UInt m_uiSpiralPathLength = 0; 
int *m_aiSpiralPath_X = NULL;
int *m_aiSpiralPath_Y = NULL;

void InitSpiral(UInt uiSearchRange = 16)
{
  m_uiSearchRange      = uiSearchRange;
  m_uiSpiralPathLength = (2*uiSearchRange+1)*(2*uiSearchRange+1) ;
  
  int* aiPathX = new int[m_uiSpiralPathLength];
  int* aiPathY = new int[m_uiSpiralPathLength];
  
  if (NULL == aiPathX || NULL == aiPathY)
    throw runtime_error("Cannot allocate memory");
  
  aiPathX[0] = 0;
  aiPathY[0] = 0;
  
  UInt uiIdx = 1;
  int iLocX, iLocY;
  for (int iDist = 1 ; iDist <= m_uiSearchRange ; ++iDist)
  {
    iLocY = -iDist;
    for (iLocX=-iDist ; iLocX<iDist ; ++iLocX)
    {
      aiPathX[uiIdx] = iLocX;
      aiPathY[uiIdx] = iLocY;
      ++uiIdx;
    }
    
    for (iLocY=-iDist ; iLocY<iDist ; ++iLocY)
    {
      aiPathX[uiIdx] = iLocX;
      aiPathY[uiIdx] = iLocY;
      ++uiIdx;
    }
      
    for (iLocX=iDist ; iLocX>-iDist ; --iLocX)
    {
      aiPathX[uiIdx] = iLocX;
      aiPathY[uiIdx] = iLocY;
      ++uiIdx;
    }
    
    for (iLocY=iDist ; iLocY>-iDist ; --iLocY)
    {
      aiPathX[uiIdx] = iLocX;
      aiPathY[uiIdx] = iLocY;
      ++uiIdx;
    }
  }
  
  assert(uiIdx == m_uiSpiralPathLength);
  
  m_aiSpiralPath_X = aiPathX;
  m_aiSpiralPath_Y = aiPathY;
}

void UnInitSpiral()
{
  m_uiSearchRange      = 0;
  m_uiSpiralPathLength = 0;
  
  delete[] m_aiSpiralPath_X;
  m_aiSpiralPath_X = NULL;
  delete[] m_aiSpiralPath_Y;
  m_aiSpiralPath_Y = NULL;
}

UInt GetPathLength()
{
  return m_uiSpiralPathLength;
}

int GetPathX(UInt uiIdx)
{
  if (uiIdx > GetPathLength())
    throw out_of_range("GetPathX index out of range");
  return m_aiSpiralPath_X[uiIdx];
}

int GetPathY(UInt uiIdx)
{
  if (uiIdx >= GetPathLength())
      throw out_of_range("GetPathY index out of range");
  return m_aiSpiralPath_Y[uiIdx];
}

int main(int argc, char* argv[])
{
  if (argc>1)
  {
    InitSpiral(atoi(argv[1]));
    for (int i=0 ; i<GetPathLength() ; ++i)
    {
        cout << "x:" << GetPathX(i) << " y:" << GetPathY(i) << endl;
    }
    UnInitSpiral();
    cout<<endl;
  }
  else
  {
    cout << "Usage: " << argv[0] << " <search-range>" << endl;
  }
  return 0;
}

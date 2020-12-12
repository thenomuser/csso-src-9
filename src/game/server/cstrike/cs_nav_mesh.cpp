//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// NavMesh.cpp
// Implementation of Navigation Mesh interface
// Author: Michael S. Booth, 2003-2004

#include "cbase.h"
#include "filesystem.h"
#include "cs_nav_mesh.h"
#include "cs_nav_node.h"
#include "cs_nav_area.h"
#include "fmtstr.h"
#include "utlbuffer.h"
#include "tier0/vprof.h"
#include "bot_util.h"
#include "cs_shareddefs.h"

#ifndef CLIENT_DLL
#include "mapinfo.h"
#endif

//--------------------------------------------------------------------------------------------------------------
CSNavMesh::CSNavMesh( void )
{
	m_desiredChickenCount = 0;
}

//--------------------------------------------------------------------------------------------------------------
CSNavMesh::~CSNavMesh()
{
}

CNavArea * CSNavMesh::CreateArea( void ) const
{
	return new CCSNavArea;
}

//-------------------------------------------------------------------------
void CSNavMesh::BeginCustomAnalysis( bool bIncremental )
{

}


//-------------------------------------------------------------------------
// invoked when custom analysis step is complete
void CSNavMesh::PostCustomAnalysis( void )
{

}


//-------------------------------------------------------------------------
void CSNavMesh::EndCustomAnalysis()
{

}


//-------------------------------------------------------------------------
/**
 * Returns sub-version number of data format used by derived classes
 */
unsigned int CSNavMesh::GetSubVersionNumber( void ) const
{
	// 1: initial implementation - added ApproachArea data
	return 1;
}

//-------------------------------------------------------------------------
/** 
 * Store custom mesh data for derived classes
 */
void CSNavMesh::SaveCustomData( CUtlBuffer &fileBuffer ) const
{

}

//-------------------------------------------------------------------------
/**
 * Load custom mesh data for derived classes
 */
void CSNavMesh::LoadCustomData( CUtlBuffer &fileBuffer, unsigned int subVersion )
{

}

//--------------------------------------------------------------------------------------------------------------
/**
 * Reset the Navigation Mesh to initial values
 */
void CSNavMesh::Reset( void )
{
	m_refreshChickenTimer.Start( 5.0f );

	CNavMesh::Reset();
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Zero player counts in all areas
 */
void CSNavMesh::ClearPlayerCounts( void )
{
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CCSNavArea *area = (CCSNavArea*)TheNavAreas[ it ];
		area->ClearPlayerCount();
	}
}


//--------------------------------------------------------------------------------------------------------------
// Keep desired number of chickens alive in map
void CSNavMesh::MaintainChickenPopulation( void )
{
	if ( m_desiredChickenCount > 0 && m_refreshChickenTimer.IsElapsed() )
	{
		m_refreshChickenTimer.Start( RandomFloat( 10.0f, 20.0f ) );

		int actualCount = 0;
		for( int i=0; i<m_chickenVector.Count(); ++i )
		{
			if ( m_chickenVector[i] != NULL )
			{
				++actualCount;
			}
		}

		if ( actualCount < m_desiredChickenCount )
		{
			int need = m_desiredChickenCount - actualCount;

			for( int k=0; k<need; ++k )
			{
				// find a good spot to spawn a chicken
				CBaseEntity *chicken = NULL;

				for( int attempts=0; attempts<10; ++attempts )
				{
					int which = RandomInt( 0, TheNavAreas.Count()-1 );

					CNavArea *testArea = TheNavAreas[ which ];

					const float tooSmall = 50.0f;

					if ( testArea && testArea->GetSizeX() > tooSmall && testArea->GetSizeY() > tooSmall )
					{
						if ( !UTIL_IsVisibleToTeam( testArea->GetCenter(), TEAM_CT ) &&
							 !UTIL_IsVisibleToTeam( testArea->GetCenter(), TEAM_TERRORIST ) )
						{
							// don't spawn a chicken on top of another chicken
							int n;
							for( n=0; n<m_chickenVector.Count(); ++n )
							{
								if ( m_chickenVector[n] == NULL )
									continue;

								const float tooClose = 50.0f;
								Vector between = m_chickenVector[n]->GetAbsOrigin() - testArea->GetCenter();
								if ( between.IsLengthLessThan( tooClose ) )
									break;
							}

							if ( n >= m_chickenVector.Count() )
							{
								// found a good spot - spawn a chicken here
								chicken = CreateEntityByName( "chicken" );
								if ( chicken )
								{
									chicken->SetAbsOrigin( testArea->GetCenter() );

									DispatchSpawn( chicken );
									m_chickenVector.AddToTail( chicken );
								}

								break;
							}
						}
					}
				}

				if ( !chicken )
				{
					// couldn't spawn a chicken - try again later
					return;
				}
			}
		}
	}
}


void CSNavMesh::Update( void )
{
	MaintainChickenPopulation();

	CNavMesh::Update();
}

NavErrorType CSNavMesh::Load( void )
{
	return CNavMesh::Load();
}

bool CSNavMesh::Save( void ) const
{
	return CNavMesh::Save();
}

NavErrorType CSNavMesh::PostLoad( unsigned int version )
{
	m_desiredChickenCount = g_pMapInfo ? g_pMapInfo->m_iPetPopulation : 0;
	m_chickenVector.RemoveAll();

	return CNavMesh::PostLoad(version);
}
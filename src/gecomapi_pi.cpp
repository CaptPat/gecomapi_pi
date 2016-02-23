/******************************************************************************
 * $Id: gecomapi_pi.cpp,v 1.0 2011/02/26 01:54:37 nohal Exp $
 *
 * Project:  OpenCPN
 * Purpose:  GoogleEarth Plugin
 * Author:   Pavel Kalian
 *
 ***************************************************************************
 *   Copyright (C) 2011 by Pavel Kalian   *
 *   $EMAIL$   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.             *
 ***************************************************************************
 */


#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers

#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>

#include "gecomapi_pi.h"
#include "gecomapi.h"


// the class factories, used to create and destroy instances of the PlugIn

extern "C" DECL_EXP opencpn_plugin* create_pi(void *ppimgr)
{
    return (opencpn_plugin *)new gecomapi_pi(ppimgr);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin* p)
{
    delete p;
}

//---------------------------------------------------------------------------------------------------------
//
//    POI Manager PlugIn Implementation
//
//---------------------------------------------------------------------------------------------------------

#include "icons.h"

//---------------------------------------------------------------------------------------------------------
//
//          PlugIn initialization and de-init
//
//---------------------------------------------------------------------------------------------------------

gecomapi_pi::gecomapi_pi(void *ppimgr)
      :opencpn_plugin_113(ppimgr)
{
      // Create the PlugIn icons
      initialize_images();

      m_pgecomapi_window = NULL;
}

int gecomapi_pi::Init(void)
{
      m_bshuttingDown = false;

      mPriPosition = 99;

      m_pgecomapi_window = NULL;

      AddLocaleCatalog( _T("opencpn-gecomapi_pi") );

      //    Get a pointer to the opencpn display canvas, to use as a parent for the POI Manager dialog
      m_parent_window = GetOCPNCanvasWindow();

      m_pauimgr = GetFrameAuiManager();

      //    Get a pointer to the opencpn configuration object
      m_pconfig = GetOCPNConfigObject();

      //    And load the configuration items
      LoadConfig();

      //    This PlugIn needs a toolbar icon
#ifdef GECOMAPI_USE_SVG
	  m_toolbar_item_id = InsertPlugInToolSVG(_T("GoogleEarth"), _svg_gecomapi, _svg_gecomapi_rollover, _svg_gecomapi_toggled, wxITEM_CHECK,
		  _T("GoogleEarth"), _T(""), NULL, GECOMAPI_TOOL_POSITION, 0, this);
#else           
	  m_toolbar_item_id = InsertPlugInTool(_T("GoogleEarth"), _img_gecomapi, _img_gecomapi, wxITEM_CHECK,
		  _T("GoogleEarth"), _T(""), NULL, GECOMAPI_TOOL_POSITION, 0, this); 
#endif

      m_pgecomapi_window = new GEUIDialog(GetOCPNCanvasWindow(), wxID_ANY, m_pauimgr, m_toolbar_item_id, this);

      wxAuiPaneInfo pane = wxAuiPaneInfo().Name(_T("GoogleEarth")).Caption(_T("GoogleEarth")).CaptionVisible(true).Float().FloatingPosition(0,0).Show(!m_bstartHidden).TopDockable(false).BottomDockable(false).LeftDockable(true).RightDockable(true).CaptionVisible(true).CloseButton(false).MinSize(300,300);

      m_pauimgr->AddPane(m_pgecomapi_window, pane);
      
      if(m_pgecomapi_window)
      {
            m_pgecomapi_window->SetCameraParameters(m_iCameraAzimuth, m_iCameraTilt, m_iCameraRange);
      }

      ApplyConfig();

      m_pauimgr->Update();

      //m_pauimgr->Connect( wxEVT_AUI_RENDER, wxAuiManagerEventHandler( gecomapi_pi::OnAuiRender ), NULL, this );

      return (WANTS_OVERLAY_CALLBACK |
           WANTS_CURSOR_LATLON       |
           WANTS_TOOLBAR_CALLBACK    |
           INSTALLS_TOOLBAR_TOOL     |
           WANTS_PREFERENCES         |
           WANTS_CONFIG              |
           WANTS_NMEA_EVENTS         |
           WANTS_NMEA_SENTENCES      |
           USES_AUI_MANAGER          |
           WANTS_ONPAINT_VIEWPORT
            );      
}

bool gecomapi_pi::DeInit(void)
{
      m_bshuttingDown = true;
      if(m_pgecomapi_window)
      {
            m_iWindowWidth = m_pgecomapi_window->GetSize().GetWidth();
            m_iWindowHeight = m_pgecomapi_window->GetSize().GetHeight();
            m_bWindowFloating = m_pauimgr->GetPane(m_pgecomapi_window).IsFloating();
            m_pauimgr->ClosePane(m_pauimgr->GetPane(m_pgecomapi_window));
            m_pauimgr->DetachPane(m_pgecomapi_window);
            //m_pgecomapi_window->GEClose();
            m_pgecomapi_window->Close();
            m_pgecomapi_window->Destroy();
            SaveConfig();
            m_pauimgr->Update();
      }
      return true;
}

int gecomapi_pi::GetAPIVersionMajor()
{
      return MY_API_VERSION_MAJOR;
}

int gecomapi_pi::GetAPIVersionMinor()
{
      return MY_API_VERSION_MINOR;
}

int gecomapi_pi::GetPlugInVersionMajor()
{
      return PLUGIN_VERSION_MAJOR;
}

int gecomapi_pi::GetPlugInVersionMinor()
{
      return PLUGIN_VERSION_MINOR;
}

wxBitmap *gecomapi_pi::GetPlugInBitmap()
{
      return _img_gecomapi_pi;
}

wxString gecomapi_pi::GetCommonName()
{
      return _T("GoogleEarth");
}


wxString gecomapi_pi::GetShortDescription()
{
      return _("GoogleEarth PlugIn for OpenCPN");
}

wxString gecomapi_pi::GetLongDescription()
{
      return _("GoogleEarth PlugIn for OpenCPN\nConquers the world");
}

void gecomapi_pi::SetCursorLatLon(double lat, double lon)
{
      if (m_bshuttingDown)
            return;
      if(m_iWhatToFollow == GECOMAPI_FOLLOW_CURSOR && m_pgecomapi_window)
      {
            m_pgecomapi_window->SetCameraParameters(m_iCameraAzimuth, m_iCameraTilt, m_iCameraRange);
            m_pgecomapi_window->SetCursorLatLon(lat, lon);
      }
}

void gecomapi_pi::SetDefaults(void)
{
}


int gecomapi_pi::GetToolbarToolCount(void)
{
      return 1;
}

void gecomapi_pi::OnToolbarToolCallback(int id)
{
      if(NULL == m_pgecomapi_window)
            return;

      wxAuiPaneInfo &pane = m_pauimgr->GetPane(m_pgecomapi_window);
      if(!pane.IsOk())
            return;
      m_pgecomapi_window->m_ballowStart = true;

      pane.Show(!pane.IsShown());
      // Toggle is handled by the toolbar but we must keep plugin manager b_toggle updated
      // to actual status to ensure right status upon toolbar rebuild
      SetToolbarItemState(m_toolbar_item_id, pane.IsShown());
      
      if (NULL != m_pgecomapi_window->app)
      {
            if (!pane.IsShown())
            {
                  m_iWindowWidth = m_pgecomapi_window->GetSize().GetWidth();
                  m_iWindowHeight = m_pgecomapi_window->GetSize().GetHeight();
                  pane.window->SetSize(m_iWindowWidth, m_iWindowHeight);
                  pane.BestSize(m_iWindowWidth, m_iWindowHeight);
                  m_pgecomapi_window->GEAttachWindow(false);
            }
            else
            {
                    pane.window->SetSize(m_iWindowWidth, m_iWindowHeight);
                    pane.BestSize(m_iWindowWidth, m_iWindowHeight);
                    m_pgecomapi_window->GEAttachWindow(true);
            }
      }
      else
      {
            if (pane.IsShown())
            {
                  m_pgecomapi_window->m_pfocusedwindow = wxWindow::FindFocus();
                  m_pgecomapi_window->GEInitialize();
                  ApplyConfig();
            }
      }
      m_pauimgr->Update();
}



void gecomapi_pi::UpdateAuiStatus(void)
{
      //    This method is called after the PlugIn is initialized
      //    and the frame has done its initial layout, possibly from a saved wxAuiManager "Perspective"
      //    It is a chance for the PlugIn to syncronize itself internally with the state of any Panes that
      //    were added to the frame in the PlugIn ctor.

      //    We use this callback here to keep the context menu selection in sync with the window state

      wxAuiPaneInfo &pane = m_pauimgr->GetPane(m_pgecomapi_window);
      if(!pane.IsOk())
            return;

      if (m_bstartHidden)
            pane.Show(false); 
      else if (pane.IsShown())
            m_pgecomapi_window->GEInitialize();
      if (pane.IsFloating())
            pane.frame->SetTransparent(m_iOpacity);
      m_pgecomapi_window->SetSize(m_iWindowWidth, m_iWindowHeight);
      pane.BestSize(m_iWindowWidth, m_iWindowHeight);
      m_pauimgr->Update();

      SetToolbarItemState(m_toolbar_item_id, pane.IsShown());
}

void gecomapi_pi::SetCurrentViewPort(PlugIn_ViewPort &vp)
{
      if (vp.clat == m_pastVp.clat && vp.clon == m_pastVp.clon && vp.pix_height == m_pastVp.pix_height && vp.pix_width == m_pastVp.pix_width && vp.rotation == m_pastVp.rotation && vp.chart_scale == m_pastVp.chart_scale && 
            vp.lat_max == m_pastVp.lat_max && vp.lat_min == m_pastVp.lat_min && vp.lon_max == m_pastVp.lon_max && vp.lon_min == m_pastVp.lon_min && vp.view_scale_ppm == m_pastVp.view_scale_ppm)
      {
            return; //Prevents event storm killing the responsiveness. At least in course-up it looks needed.
      }
      m_pastVp = vp;
      if (m_bshuttingDown)
            return;
      LogDebugMessage(_T("SetCurrentViewPort called by OpenCPN"));
      if(m_iWhatToFollow == GECOMAPI_FOLLOW_VIEW && m_pgecomapi_window)
      {
            m_pgecomapi_window->SetViewPort(vp.clat, vp.clon, vp.lat_max - vp.lat_min, vp.lon_max - vp.lon_min, vp.rotation);
      }
}

void gecomapi_pi::SetPositionFix(PlugIn_Position_Fix &pfix)
{
      if (m_bshuttingDown)
            return;
      if ( m_pgecomapi_window )
      {
            m_pgecomapi_window->ShowBoat(pfix.Lat, pfix.Lon);
      }
      if(m_iWhatToFollow == GECOMAPI_FOLLOW_BOAT && m_pgecomapi_window)
      {
            if (mPriPosition >= 1)
            {
                  mPriPosition = 1;
                  //m_pgecomapi_window->SetCameraParameters(m_iCameraAzimuth, m_iCameraTilt, m_iCameraRange);
                  m_pgecomapi_window->SetBoatLatLon(pfix.Lat, pfix.Lon);
            }
      }
}

void gecomapi_pi::SetParams(double azimuth, double range, double tilt)
{
      m_iCameraAzimuth = (int) azimuth;
      m_iCameraTilt = (int) tilt;
      m_iCameraRange = (int) range;
}

bool gecomapi_pi::LoadConfig(void)
{
      wxFileConfig *pConf = (wxFileConfig *)m_pconfig;

      if(pConf)
      {
            pConf->SetPath( _T("/PlugIns/GoogleEarth") );

            wxString config;

            pConf->Read( _T( "WindowWidth" ), &m_iWindowWidth, DEFAULT_WIDTH );
            pConf->Read( _T( "WindowHeight" ), &m_iWindowHeight, DEFAULT_HEIGHT );
            pConf->Read( _T( "WindowFloating" ), &m_bWindowFloating, DEFAULT_FLOATING );
            pConf->Read( _T( "AlwaysStartHidden" ), &m_bstartHidden, true );
            pConf->Read( _T( "DisconnectOnGEAction" ), &m_bdisconnectOnGEAction, true );
            pConf->Read( _T( "UpdateSettingsFromGE" ), &m_bupdateSettingsFromGE, true );
            pConf->Read( _T( "ShowBoatInGE" ), &m_bShowBoatInGE, true );
            pConf->Read( _T( "WhatToFollow" ), &m_iWhatToFollow, GECOMAPI_FOLLOW_VIEW ); //1-Cursor, 2-boat, 3-View, 0-Nothing
            pConf->Read( _T( "CameraAzimuth" ), &m_iCameraAzimuth, 0 );
            pConf->Read( _T( "CameraTilt" ), &m_iCameraTilt, 0 );
            pConf->Read( _T( "CameraRange" ), &m_iCameraRange, 0 );
            pConf->Read( _T( "WindowOpacity" ), &m_iOpacity, 255 );
            LogDebugMessage(wxString::Format(_T("Config params loaded: WW=%d, ASH=%d, DGA=%d, USG=%d, SB=%d, WF=%d, Az=%d, Ti=%d, Ra=%d, Opa=%d"), 
                  m_iWindowWidth, m_bstartHidden, m_bdisconnectOnGEAction, m_bupdateSettingsFromGE, m_bShowBoatInGE, 
                  m_iWhatToFollow, m_iCameraAzimuth, m_iCameraTilt, m_iCameraRange, m_iOpacity));
            return true;
      }
      else
            return false;
}

bool gecomapi_pi::SaveConfig(void)
{
      wxFileConfig *pConf = (wxFileConfig *)m_pconfig;

      if(pConf)
      {
            pConf->SetPath( _T( "/PlugIns/GoogleEarth" ) );
            pConf->Write( _T( "WindowWidth" ), m_iWindowWidth );
            pConf->Write( _T( "WindowHeight" ), m_iWindowHeight );
            pConf->Write( _T( "WindowFloating" ), m_bWindowFloating );
            pConf->Write( _T( "AlwaysStartHidden" ), m_bstartHidden );
            pConf->Write( _T( "DisconnectOnGEAction" ), m_bdisconnectOnGEAction );
            pConf->Write( _T( "UpdateSettingsFromGE" ), m_bupdateSettingsFromGE );
            pConf->Write( _T( "ShowBoatInGE" ), m_bShowBoatInGE );
            pConf->Write( _T( "WhatToFollow" ), m_iWhatToFollow );
            pConf->Write( _T( "CameraAzimuth" ), m_iCameraAzimuth );
            pConf->Write( _T( "CameraTilt" ), m_iCameraTilt );
            pConf->Write( _T( "CameraRange" ), m_iCameraRange );
            pConf->Write( _T( "WindowOpacity" ), m_iOpacity );
            LogDebugMessage(wxString::Format(_T("Config params saved: WW=%d, ASH=%d, DGA=%d, USG=%d, SB=%d, WF=%d, Az=%d, Ti=%d, Ra=%d, Opa=%d"), 
                  m_iWindowWidth, m_bstartHidden, m_bdisconnectOnGEAction, m_bupdateSettingsFromGE, m_bShowBoatInGE, 
                  m_iWhatToFollow, m_iCameraAzimuth, m_iCameraTilt, m_iCameraRange, m_iOpacity));
            return true;
      }
      else
            return false;
}

void gecomapi_pi::ApplyConfig(void)
{
      wxAuiPaneInfo &pane = m_pauimgr->GetPane(m_pgecomapi_window);
      if(!pane.IsOk())
            return;
      if (pane.IsFloating())
      {
            if ( pane.frame )
                  pane.frame->SetTransparent(m_iOpacity);
      }
      m_pgecomapi_window->SetSize(m_iWindowWidth, m_iWindowHeight);
      pane.BestSize(m_iWindowWidth, m_iWindowHeight);
}


void gecomapi_pi::ShowPreferencesDialog( wxWindow* parent )
{
      GEPrefsDlg *dialog = new GEPrefsDlg( parent, wxID_ANY );

      dialog->m_Azimuthslider->SetValue(m_iCameraAzimuth);
      dialog->m_Tiltslider->SetValue(m_iCameraTilt);
      dialog->m_Rangeslider->SetValue(m_iCameraRange);
      dialog->m_Transparencyslider->SetValue(m_iOpacity);
      dialog->m_cbStartHidden->SetValue(m_bstartHidden);
      dialog->m_cbAutodisconnect->SetValue(m_bdisconnectOnGEAction);
      dialog->m_cbUpdateSettingsFromGE->SetValue(m_bupdateSettingsFromGE);
      dialog->m_cbShowBoatInGE->SetValue(m_bShowBoatInGE);
      switch(m_iWhatToFollow)
      {
            case GECOMAPI_FOLLOW_BOAT:
                  dialog->m_radioFlwBoat->SetValue(true);
                  break;
            case GECOMAPI_FOLLOW_CURSOR:
                  dialog->m_radioFlwCursor->SetValue(true);
                  break;
            case GECOMAPI_FOLLOW_VIEW:
                  dialog->m_radioFlwView->SetValue(true);
                  dialog->m_Azimuthslider->Disable();
                  dialog->m_Tiltslider->Disable();
                  dialog->m_Rangeslider->Disable();
                  dialog->m_btnResetToDefaults->Disable();
                  break;
            case GECOMAPI_FOLLOW_OFF:
                  dialog->m_radioDontFollow->SetValue(true);
                  break;
      }

      if(dialog->ShowModal() == wxID_OK)
      {
            m_iCameraAzimuth = dialog->m_Azimuthslider->GetValue();
            m_iCameraTilt = dialog->m_Tiltslider->GetValue();
            m_iCameraRange = dialog->m_Rangeslider->GetValue();
            m_iOpacity = dialog->m_Transparencyslider->GetValue();
            m_bstartHidden = dialog->m_cbStartHidden->GetValue();
            m_bdisconnectOnGEAction = dialog->m_cbAutodisconnect->GetValue();
            m_bupdateSettingsFromGE = dialog->m_cbUpdateSettingsFromGE->GetValue();
            m_bShowBoatInGE = dialog->m_cbShowBoatInGE->GetValue();
            if (dialog->m_radioFlwBoat->GetValue())
                  m_iWhatToFollow = GECOMAPI_FOLLOW_BOAT;
            if (dialog->m_radioFlwCursor->GetValue())
                  m_iWhatToFollow = GECOMAPI_FOLLOW_CURSOR;
            if (dialog->m_radioFlwView->GetValue())
                  m_iWhatToFollow = GECOMAPI_FOLLOW_VIEW;
            if (dialog->m_radioDontFollow->GetValue())
                  m_iWhatToFollow = GECOMAPI_FOLLOW_OFF;

            SaveConfig();
            ApplyConfig();
            // WAS IST DAS? SetToolbarItemState( m_toolbar_item_id, GetDashboardWindowShownCount()==0 );
      }
      dialog->Destroy();
}


void gecomapi_pi::UpdateFromGE(double azimuth, double tilt, double range)
{
      m_iCameraAzimuth = (int) azimuth;
      m_iCameraTilt = (int) tilt;
      m_iCameraRange = (int) range;

      SaveConfig();
}

bool gecomapi_pi::KillProcessByName(wxString szProcessToKill)
{
	HANDLE hProcessSnap;
	HANDLE hProcess;
	PROCESSENTRY32 pe32;

	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);  // Takes a snapshot of all the processes

	if(hProcessSnap == INVALID_HANDLE_VALUE){
		return( FALSE );
	}

	pe32.dwSize = sizeof(PROCESSENTRY32);

	if(!Process32First(hProcessSnap, &pe32)){
		CloseHandle(hProcessSnap);     
		return( FALSE );
	}

	do{
            if(!wcscmp(pe32.szExeFile, szProcessToKill.c_str())){    //  checks if process at current position has the name of to be killed app
			hProcess = OpenProcess(PROCESS_TERMINATE,0, pe32.th32ProcessID);  // gets handle to process
			TerminateProcess(hProcess,0);   // Terminate process by handle
			CloseHandle(hProcess);  // close the handle
		} 
	}while(Process32Next(hProcessSnap,&pe32));  // gets next member of snapshot

	CloseHandle(hProcessSnap);  // closes the snapshot handle
	return( TRUE );
}

void gecomapi_pi::OnAuiRender( wxAuiManagerEvent& event )
{
      //TODO: Can we prevent the resizing here?
      event.Skip();
}

bool gecomapi_pi::IsProcessRunningByName(wxString szProcessToFind)
{
	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;
      bool result = false;

	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);  // Takes a snapshot of all the processes

	if(hProcessSnap == INVALID_HANDLE_VALUE){
		return( false );
	}

	pe32.dwSize = sizeof(PROCESSENTRY32);

	if(!Process32First(hProcessSnap, &pe32)){
		CloseHandle(hProcessSnap);     
		return( false );
	}

	do{
            if(!wcscmp(pe32.szExeFile, szProcessToFind.c_str())){    //  checks if process at current position has the name of to be killed app
			result = true;
                  break;
		} 
	}while(Process32Next(hProcessSnap,&pe32));  // gets next member of snapshot

	CloseHandle(hProcessSnap);  // closes the snapshot handle
	return( result );
}

void GEPrefsDlg::ResetToDefaults(wxCommandEvent& event)
{
      m_Azimuthslider->SetValue(0);
      m_Tiltslider->SetValue(0);
      m_Rangeslider->SetValue(10000);

      event.Skip();
}

void GEPrefsDlg::BtnTogled(wxCommandEvent& event)
{
      if(m_radioFlwView->GetValue())
      {
            m_Azimuthslider->Disable();
            m_Tiltslider->Disable();
            m_Rangeslider->Disable();
            m_btnResetToDefaults->Disable();
      }
      else
      {
            m_Azimuthslider->Enable();
            m_Tiltslider->Enable();
            m_Rangeslider->Enable();
            m_btnResetToDefaults->Enable();
      }

      event.Skip();
}

void LogDebugMessage(wxString msg)
{
      if (DEBUG_MESSAGES)
            wxLogMessage(_T("[GECOMAPI] %s"), msg.c_str());
}
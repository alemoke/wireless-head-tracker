#include "stdafx.h"
#pragma hdrstop

#include "resource.h"

#include "../dongle/reports.h"

#include "my_utils.h"
#include "my_win.h"
#include "d3d.h"
#include "d3d_objects.h"
#include "wht_dongle.h"
#include "mag_calib_dialog.h"

MagCalibDialog::MagCalibDialog(WHTDongle& dngl)
:	_icon_large(IDI_MAGNET, true),
	_icon_small(IDI_MAGNET, false),
	_camera(_d3d_device),
	_is_dragging(false),
	_dongle(dngl),
	_num_samples(0)
{}

void MagCalibDialog::OnInit()
{
	SetIcon(_icon_small);
	SetIcon(_icon_large);

	_btn_clear_points.SetHandle(GetCtrl(IDC_BTN_CLEAR_POINTS));
	_btn_reset_camera.SetHandle(GetCtrl(IDC_BTN_RESET_CAMERA));

	_lbl_num_points.SetHandle(GetCtrl(IDC_LBL_NUM_POINTS));
	_lbl_num_samples.SetHandle(GetCtrl(IDC_LBL_NUM_SAMPLES));

	_d3d_window.SetHandle(GetCtrl(IDC_D3D));
	
	UpdateD3DSize();

	// init the Direct3D device
	_d3d_device.Init(_d3d, _d3d_window);
	Init3D();

	::SetTimer(GetHandle(), 1, 100, NULL);
}

void MagCalibDialog::OnException(const std::wstring& str)
{
	MsgBox(str, L"Exception", MB_OK | MB_ICONERROR);
}

void MagCalibDialog::OnDestroy()
{
	_hWnd = 0;

	// release all our D3D objects except the Direct3D
	_d3d_device.Release();
	_coord_sys.Release();

	ClearSamples();
}

void MagCalibDialog::ClearSamples()
{
	_mags.clear();
	_mag_set.clear();
	_num_samples = 0;
}

void MagCalibDialog::OnSize(int width, int height, WPARAM wParam)
{
	WaitCursor cwc;

	UpdateD3DSize();

	// re-init Direct3D
	std::for_each(_mags.begin(), _mags.end(), [&](MagPoint& m) { m.Release(); } );

	_coord_sys.Release();
	_d3d_device.Release();

	_d3d_device.Init(_d3d, _d3d_window);
	
	Init3D();
}

void MagCalibDialog::UpdateD3DSize()
{
	RECT dlgrect;
	int dlgw, dlgh, dxw, dxh;
	
	GetRect(dlgrect);
	dlgw = dlgrect.right - dlgrect.left;
	dlgh = dlgrect.bottom - dlgrect.top;

	dxw = dlgw - 30;
	dxh = dlgh - 87;

	// set the D3D window size
	::SetWindowPos(_d3d_window.GetHandle(), HWND_TOP, 0, 0, dxw, dxh, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}

void MagCalibDialog::Render()
{
    // return if the device is not ready
    if (!_d3d_device.IsValid())
		return;

    // Clear the back buffer
	_d3d_device.Clear();

	_d3d_device.BeginScene();

	// should we use light?
	//if (UseLighting())
	//	_d3d_device.EnableLight();
	//else
	//	_d3d_device.DisableLight();

	// set the current view transformation
	_camera.RefreshPos();

	// render our objects
	_coord_sys.Render(_d3d_device);

	std::for_each(_mags.begin(), _mags.end(), [&](MagPoint& m) {m.Render(_d3d_device); } );

	_d3d_device.EndScene();

    // present the back buffer to the display adapter to be drawn
    _d3d_device.Present();
}

void MagCalibDialog::Init3D()
{
	// Create a matrix to store our Projection transform. Null all the fields.
	D3DXMATRIX matProjection;
	ZeroMemory(&matProjection, sizeof(matProjection));

	RECT r;
	GetRect(r);

	// Use D3DX to create a left handed cartesian Field Of View transform
	D3DXMatrixPerspectiveFovLH(	&matProjection,
								D3DX_PI / 4,
								(float) (r.right - r.left) / (r.bottom - r.top),
								1.0,
								10000.0);
	
	// Tell D3D to use our Projection matrix for the projection transformation stage
	_d3d_device.SetProjectionTransform(matProjection);

	_d3d_device.SetCulling(D3DCULL_NONE);
	_d3d_device.DisableLight();

	_coord_sys.Build(_d3d_device);

	// this one's not really necessary, but it won't hurt either
	//_mags.reserve(2000);
}

void MagCalibDialog::OnTimer(int timerID)
{
	// get raw the mag data
	FeatRep_MagRawData repMagData;
	repMagData.report_id = MAG_RAW_DATA_REPORT_ID;
	_dongle.GetFeatureReport(repMagData);

	_num_samples += repMagData.num_samples;

	for (int i = 0; i < repMagData.num_samples; ++i)
	{
		mag_point_t mps;
		mps.x = repMagData.mag[i].x;
		mps.y = repMagData.mag[i].y;
		mps.z = repMagData.mag[i].z;

		if (_mag_set.find(mps) == _mag_set.end())
		{
			MagPoint mp;
			mp.Build(_d3d_device, repMagData.mag[i].x, repMagData.mag[i].y, repMagData.mag[i].z);

			_mags.push_back(mp);

			_mag_set.insert(mps);
		}
	}

	// update the counters
	_lbl_num_points.SetText((int) _mags.size());
	_lbl_num_samples.SetText(_num_samples);
}

void MagCalibDialog::OnControl(int ctrlID, int notifyID, HWND hWndCtrl)
{
	if (ctrlID == IDC_BTN_CLEAR_POINTS)
		ClearSamples();
	else if (ctrlID == IDC_BTN_RESET_CAMERA)
		_camera.Reset();
	else if (ctrlID == IDC_BTN_SAVE)
		SaveData();
	else if (ctrlID == IDC_BTN_LOAD)
		LoadData();
}

void MagCalibDialog::OnLButtonDown(int x, int y, WPARAM wParam)
{
	// TODO: proper mouse capture
	_is_dragging = true;
	_last_x = x;
	_last_y = y;
}

void MagCalibDialog::OnLButtonUp(int x, int y, WPARAM wParam)
{
	_is_dragging = false;
}

void MagCalibDialog::OnMouseMove(int x, int y, WPARAM wParam)
{
	if (_is_dragging)
	{
		_camera.SetRotation(float(x - _last_x), float(y - _last_y));

		_last_x = x;
		_last_y = y;
	}
}

void MagCalibDialog::OnMouseWheel(int x, int y, int delta, WPARAM wParam)
{
	_camera.Zoom(delta);
}

void MagCalibDialog::SaveData()
{
	OpenSaveFileDialog saveFile;
	saveFile.AddFilter(L"CSV file (*.csv)", L"*.csv");
	saveFile.AddFilter(L"All files (*.*)", L"*.*");
	saveFile.SetDefaultFileName(L"magdata.csv");

	if (saveFile.GetSaveFile(L"Save magnetometer samples", *this))
	{
		std::ofstream outf(saveFile.GetFullFileName().c_str(), std::ios_base::trunc);

		if (outf.is_open())
		{
			for (std::vector<MagPoint>::iterator mi(_mags.begin()); mi != _mags.end(); ++mi)
				outf << mi->x << ',' << mi->y << ',' << mi->z << std::endl;
		} else {
			MsgBox(L"Unable to open file " + saveFile.GetFullFileName(), L"Error", MB_OK | MB_ICONERROR);
		}
	}
}

void MagCalibDialog::LoadData()
{
	OpenSaveFileDialog openFile;
	openFile.AddFilter(L"CSV file (*.csv)", L"*.csv");
	openFile.AddFilter(L"All files (*.*)", L"*.*");
	openFile.SetDefaultFileName(L"magdata.csv");

	if (openFile.GetOpenFile(L"Load magnetometer samples", *this))
	{
		std::ifstream inf(openFile.GetFullFileName().c_str());

		if (inf.is_open())
		{
			ClearSamples();

			while (!inf.eof())
			{
				MagPoint mp;

				std::string line;
				std::getline(inf, line);

				const char* pstart = line.c_str();
				char* pend;

				mag_point_t mps;

				mps.x = (int16_t) strtol(pstart, &pend, 10);
				if (pstart == pend)
					break;
				pstart = pend + 1;

				mps.y = (int16_t) strtol(pstart, &pend, 10);
				if (pstart == pend)
					break;
				pstart = pend + 1;

				mps.z = (int16_t) strtol(pstart, NULL, 10);
				if (pstart == pend)
					break;

				if (_mag_set.find(mps) == _mag_set.end())
				{
					_mag_set.insert(mps);

					mp.Build(_d3d_device, mps.x, mps.y, mps.z);
					_mags.push_back(mp);
				}
			}

			_num_samples = _mags.size();
		} else {
			MsgBox(L"Unable to open file " + openFile.GetFullFileName(), L"Error", MB_OK | MB_ICONERROR);
		}
	}
}
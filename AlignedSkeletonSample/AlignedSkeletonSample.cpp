#ifndef STRICT
#define STRICT	// �����ȃR�[�h���^��v������
#endif
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <k4a/k4a.h>
#include <k4abt.h>
#include <k4arecord/playback.h>

#define PLAYBACK_FILENAME	"x.mkv"	//nullptr

#pragma comment( lib, "k4a.lib" )
#pragma comment( lib, "k4abt.lib" )
#pragma comment( lib, "k4arecord.lib" )

#define MAX_BODIES				8

int g_iWidth = 1024;
int g_iHeight = 1024;

// �A�v���P�[�V�����̃^�C�g����
static const TCHAR szClassName[] = TEXT("���i���W��[�x���W�ɍ��킹��T���v��");
HWND g_hWnd = NULL;							// �A�v���P�[�V�����̃E�B���h�E
HPEN g_hPen = NULL;							// �`��p�̃y��

HBITMAP g_hBMP = NULL, g_hBMPold = NULL;	// �\������r�b�g�}�b�v�̃n���h��
HDC g_hDCBMP = NULL;						// �\������r�b�g�}�b�v�̃R���e�L�X�g
BITMAPINFO g_biBMP = { 0, };				// �r�b�g�}�b�v�̏�� (�𑜓x��t�H�[�}�b�g)
LPDWORD g_pdwPixel = NULL;					// �r�b�g�}�b�v�̒��g�̐擪 (�s�N�Z�����)
LPDWORD g_pColorMap = NULL;					// �J���[�}�b�v�o�b�t�@�̃|�C���^

k4a_device_t g_hAzureKinect = nullptr;		// Azure Kinect �̃f�o�C�X�n���h��
k4abt_tracker_t g_hTracker = nullptr;		// �{�f�B�g���b�J�[�̃n���h��
k4a_calibration_t g_Calibration;			// Azure Kinect �̃L�����u���[�V�����f�[�^
k4abt_skeleton_t g_Skeleton[MAX_BODIES];	// ���[�U�[�� 3D ���i���
k4a_float2_t g_fSkeleton2D[MAX_BODIES][K4ABT_JOINT_COUNT] = { 0.0f, };	// ���[�U�[�� 2D ���i���W (�\���p)
uint32_t g_uBodyID[MAX_BODIES] = { K4ABT_INVALID_BODY_ID, };
uint32_t g_uBodies = 0;						// ���i�ǐՂ���Ă���l��

k4a_playback_t g_hPlayback = nullptr;
k4a_record_configuration_t g_RecordConfig;

// Kinect ���I������
void DestroyKinect( bool bPlayback )
{
	// ���i�ǐՂ𖳌��ɂ���
	if ( g_hTracker )
	{
		k4abt_tracker_destroy( g_hTracker );
		g_hTracker = nullptr;
	}

	if ( g_hPlayback )
	{
		k4a_playback_close( g_hPlayback );
		g_hPlayback = nullptr;
	}
	if ( g_hAzureKinect )
	{
		// Azure Kinect ���~����
		k4a_device_stop_cameras( g_hAzureKinect );

		// Azure Kinect �̎g�p����߂�
		k4a_device_close( g_hAzureKinect );
		g_hAzureKinect = nullptr;
	}
}

// Kinect ������������
k4a_result_t CreateKinect( const char* pRecordingFileName )
{
	k4a_result_t hr;

	// Azure Kinect ������������
	if ( pRecordingFileName )
	{
		hr = k4a_playback_open( pRecordingFileName, &g_hPlayback );
	}
	else
	{
		hr = k4a_device_open( K4A_DEVICE_DEFAULT, &g_hAzureKinect );
	}

	if ( hr == K4A_RESULT_SUCCEEDED )
	{
		k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
		if ( pRecordingFileName )
		{
			hr = k4a_playback_get_record_configuration( g_hPlayback, &g_RecordConfig );
			if ( hr == K4A_RESULT_SUCCEEDED )
			{
				// �t�@�C���ɋL�^����Ă���ݒ���f�o�C�X�\���ɃR�s�[
				config.camera_fps = g_RecordConfig.camera_fps;
				config.color_format = g_RecordConfig.color_format;
				config.color_resolution = g_RecordConfig.color_resolution;
				config.depth_delay_off_color_usec = g_RecordConfig.depth_delay_off_color_usec;
				config.depth_mode = g_RecordConfig.depth_mode;
				config.subordinate_delay_off_master_usec = g_RecordConfig.subordinate_delay_off_master_usec;
				config.wired_sync_mode = g_RecordConfig.wired_sync_mode;

				// �L�����u���[�V�����f�[�^���擾����
				hr = k4a_playback_get_calibration( g_hPlayback, &g_Calibration );
			}
			else
			{
				MessageBox( NULL, TEXT("Azure Kinect �t�@�C���Đ����J�n�ł��܂���ł���"), TEXT("�G���["), MB_OK );
			}
		}
		else
		{
			// �J�������J�n����
			config.color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
			config.color_resolution = K4A_COLOR_RESOLUTION_OFF;
			config.depth_mode = K4A_DEPTH_MODE_WFOV_2X2BINNED;
			config.camera_fps = K4A_FRAMES_PER_SECOND_30;
			config.synchronized_images_only = false;
			config.depth_delay_off_color_usec = 0;
			config.wired_sync_mode = K4A_WIRED_SYNC_MODE_STANDALONE;
			config.subordinate_delay_off_master_usec = 0;
			config.disable_streaming_indicator = false;

			// Azure Kinect ���J�n����
			hr = k4a_device_start_cameras( g_hAzureKinect, &config );
			if ( hr == K4A_RESULT_SUCCEEDED )
			{
				// �L�����u���[�V�����f�[�^���擾����
				hr = k4a_device_get_calibration( g_hAzureKinect, config.depth_mode, config.color_resolution, &g_Calibration );
			}
			else
			{
				MessageBox( NULL, TEXT("Azure Kinect ���J�n�ł��܂���ł���"), TEXT("�G���["), MB_OK );
			}
		}

		switch( config.depth_mode )
		{
		case K4A_DEPTH_MODE_NFOV_2X2BINNED:
			g_iWidth = 320;
			g_iHeight = 288;
			break;
		case K4A_DEPTH_MODE_NFOV_UNBINNED:
			g_iWidth = 640;
			g_iHeight = 576;
			break;
		case K4A_DEPTH_MODE_WFOV_2X2BINNED:
			g_iWidth = 512;
			g_iHeight = 512;
			break;
		default:
			g_iWidth = 1024;
			g_iHeight = 1024;
			break;
		}

		if ( hr == K4A_RESULT_SUCCEEDED )
		{
			// ���i�ǐՂ��J�n����
			hr = k4abt_tracker_create( &g_Calibration, K4ABT_TRACKER_CONFIG_DEFAULT, &g_hTracker );
			if ( hr == K4A_RESULT_SUCCEEDED )
			{
				return hr;
			}
			else
			{
				MessageBox( NULL, TEXT("���i�ǐՂ��J�n�ł��܂���ł���"), TEXT("�G���["), MB_OK );
			}
		}
		else
		{
			MessageBox( NULL, TEXT("�L�����u���[�V���������擾�ł��܂���ł���"), TEXT("�G���["), MB_OK );
		}

		// Azure Kinect �̎g�p����߂�
		DestroyKinect( pRecordingFileName );
	}
	else
	{
		MessageBox( NULL, TEXT("Azure Kinect �̏������Ɏ��s - ���R�[�f�B���O�̏�Ԃ��m�F���Ă�������"), TEXT("�G���["), MB_OK );
	}
	return hr;
}

// KINECT �̃��C�����[�v����
uint32_t KinectProc( bool bPlayback )
{
	k4a_wait_result_t hr = K4A_WAIT_RESULT_FAILED;
	uint32_t uImageSize = 0;
	uint32_t uBodies = 0;

	k4a_capture_t hCapture = nullptr;
	// �J�����ŃL���v�`���[����
	if ( bPlayback )
	{
		k4a_stream_result_t hrs = k4a_playback_get_next_capture( g_hPlayback, &hCapture );
		hr = (k4a_wait_result_t) hrs;
	}
	else
	{
		hr = k4a_device_get_capture( g_hAzureKinect, &hCapture, K4A_WAIT_INFINITE );
	}
	if ( hr == K4A_WAIT_RESULT_SUCCEEDED )
	{
		k4a_image_t hImage;

		// �J���[�C���[�W���擾����
		hImage = k4a_capture_get_depth_image( hCapture );

		// ���i�ǐՂ��L���[����
		hr = k4abt_tracker_enqueue_capture( g_hTracker, hCapture, K4A_WAIT_INFINITE );
		// �J�����L���v�`���[���������
		k4a_capture_release( hCapture );

		if ( hImage )
		{
			// �C���[�W�s�N�Z���̐擪�|�C���^���擾����
			uint8_t* p = k4a_image_get_buffer( hImage );
			if ( p )
			{
				uint16_t* pDepth = (uint16_t*) p;
				// �C���[�W�T�C�Y���擾����
				uImageSize = (uint32_t) k4a_image_get_size( hImage ) / sizeof(uint16_t);
				for( uint32_t i = 0; i < uImageSize; i++ )
				{
					DWORD dw = pDepth[i] & 0xFF;
					g_pColorMap[i] = (dw << 24) | (dw << 16) | (dw << 8) | dw;
				}
			}
			// �C���[�W���������
			k4a_image_release( hImage );
		}

		if ( hr == K4A_WAIT_RESULT_SUCCEEDED )
		{
			// ���i�ǐՂ̌��ʂ��擾����
			k4abt_frame_t hBodyFrame = nullptr;
			hr = k4abt_tracker_pop_result( g_hTracker, &hBodyFrame, K4A_WAIT_INFINITE );
			if ( hr == K4A_WAIT_RESULT_SUCCEEDED )
			{
				// �F�����ꂽ�l�����擾����
				uBodies = k4abt_frame_get_num_bodies( hBodyFrame );
				if ( uBodies > MAX_BODIES )
					uBodies = MAX_BODIES;
				for( uint32_t uBody = 0; uBody < uBodies; uBody++ )
				{
					// �e�l�̍��i�����擾����
					if ( k4abt_frame_get_body_skeleton( hBodyFrame, uBody, &g_Skeleton[uBody] ) == K4A_RESULT_SUCCEEDED )
					{
						g_uBodyID[uBody] = k4abt_frame_get_body_id( hBodyFrame, uBody );
						for( int iJoint = K4ABT_JOINT_PELVIS; iJoint < K4ABT_JOINT_COUNT; iJoint++ )
						{
							int iValid = 0;
							// 3D �̍��i���W�� 2D �X�N���[�����W�ɕϊ�����
							k4a_calibration_3d_to_2d( &g_Calibration, &g_Skeleton[uBody].joints[iJoint].position, K4A_CALIBRATION_TYPE_DEPTH, K4A_CALIBRATION_TYPE_DEPTH, &g_fSkeleton2D[uBody][iJoint], &iValid );
							if ( iValid == 0 )
							{
								// �����Ȓl�� (0,0) �ɐݒ�
								g_fSkeleton2D[uBody][iJoint].xy.x = g_fSkeleton2D[uBody][iJoint].xy.y = 0.0f;
							}
						}
					}
				}
				// ���i�ǐՃt���[�����������
				k4abt_frame_release( hBodyFrame );
				g_uBodies = uBodies;
			}
		}
	}
	return uImageSize;
}

LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch( uMsg )
	{
	case WM_PAINT:
		{
			// ��ʕ\������
			PAINTSTRUCT ps;
			HDC hDC = BeginPaint( hWnd, &ps );

			// ��ʃT�C�Y���擾����
			RECT rect;
			GetClientRect( hWnd, &rect );

			CopyMemory( g_pdwPixel, g_pColorMap, g_iWidth * g_iHeight * sizeof(DWORD) );

			// �J���[�������[�x�̕\��
			StretchBlt( hDC, 0, 0, rect.right, rect.bottom, g_hDCBMP, 0, 0, g_iWidth, g_iHeight, SRCCOPY );

			// �[�x�f���̏c�������擾���ĉ�ʃT�C�Y�Ƃ̊��������߂�
			const DWORD dwWidth = g_iWidth;
			const DWORD dwHeight = g_iHeight;
			const FLOAT fRelativeWidth = (FLOAT) rect.right / (FLOAT) dwWidth;
			const FLOAT fRelativeHeight = (FLOAT) rect.bottom / (FLOAT) dwHeight;

			// �y����I��
			HPEN hPenPrev = (HPEN) SelectObject( hDC, g_hPen );

			// �����̔w�i�𓧖���
			SetBkMode( hDC, TRANSPARENT );

			for( uint32_t uBody = 0; uBody < g_uBodies; uBody++ )
			{
				// ���i����ʃT�C�Y�ɍ��킹�ăX�P�[�����O
				LONG lSkeletonX[K4ABT_JOINT_COUNT];
				LONG lSkeletonY[K4ABT_JOINT_COUNT];
				for( int iJoint = K4ABT_JOINT_PELVIS; iJoint < K4ABT_JOINT_COUNT; iJoint++ )
				{
					lSkeletonX[iJoint] = (LONG) (g_fSkeleton2D[uBody][iJoint].xy.x * fRelativeWidth);
					lSkeletonY[iJoint] = (LONG) (g_fSkeleton2D[uBody][iJoint].xy.y * fRelativeHeight);
				}

				// �����Ō���
				MoveToEx( hDC, lSkeletonX[K4ABT_JOINT_EAR_LEFT], lSkeletonY[K4ABT_JOINT_EAR_LEFT], NULL );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_EYE_LEFT], lSkeletonY[K4ABT_JOINT_EYE_LEFT] );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_NOSE], lSkeletonY[K4ABT_JOINT_NOSE] );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_EYE_RIGHT], lSkeletonY[K4ABT_JOINT_EYE_RIGHT] );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_EAR_RIGHT], lSkeletonY[K4ABT_JOINT_EAR_RIGHT] );

				// �̂���Ō���
				MoveToEx( hDC, lSkeletonX[K4ABT_JOINT_PELVIS], lSkeletonY[K4ABT_JOINT_PELVIS], NULL );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_SPINE_NAVEL], lSkeletonY[K4ABT_JOINT_SPINE_NAVEL] );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_SPINE_CHEST], lSkeletonY[K4ABT_JOINT_SPINE_CHEST] );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_NECK], lSkeletonY[K4ABT_JOINT_NECK] );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_HEAD], lSkeletonY[K4ABT_JOINT_HEAD] );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_NOSE], lSkeletonY[K4ABT_JOINT_NOSE] );

				// �r����Ō���
				MoveToEx( hDC, lSkeletonX[K4ABT_JOINT_HAND_LEFT], lSkeletonY[K4ABT_JOINT_HAND_LEFT], NULL );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_WRIST_LEFT], lSkeletonY[K4ABT_JOINT_WRIST_LEFT] );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_ELBOW_LEFT], lSkeletonY[K4ABT_JOINT_ELBOW_LEFT] );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_SHOULDER_LEFT], lSkeletonY[K4ABT_JOINT_SHOULDER_LEFT] );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_CLAVICLE_LEFT], lSkeletonY[K4ABT_JOINT_CLAVICLE_LEFT] );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_SPINE_CHEST], lSkeletonY[K4ABT_JOINT_SPINE_CHEST] );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_CLAVICLE_RIGHT], lSkeletonY[K4ABT_JOINT_CLAVICLE_RIGHT] );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_SHOULDER_RIGHT], lSkeletonY[K4ABT_JOINT_SHOULDER_RIGHT] );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_ELBOW_RIGHT], lSkeletonY[K4ABT_JOINT_ELBOW_RIGHT] );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_WRIST_RIGHT], lSkeletonY[K4ABT_JOINT_WRIST_RIGHT] );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_HAND_RIGHT], lSkeletonY[K4ABT_JOINT_HAND_RIGHT] );

				// �������Ō���
				MoveToEx( hDC, lSkeletonX[K4ABT_JOINT_HANDTIP_LEFT], lSkeletonY[K4ABT_JOINT_HANDTIP_LEFT], NULL );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_HAND_LEFT], lSkeletonY[K4ABT_JOINT_HAND_LEFT] );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_THUMB_LEFT], lSkeletonY[K4ABT_JOINT_THUMB_LEFT] );

				// �E�����Ō���
				MoveToEx( hDC, lSkeletonX[K4ABT_JOINT_HANDTIP_RIGHT], lSkeletonY[K4ABT_JOINT_HANDTIP_RIGHT], NULL );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_HAND_RIGHT], lSkeletonY[K4ABT_JOINT_HAND_RIGHT] );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_THUMB_RIGHT], lSkeletonY[K4ABT_JOINT_THUMB_RIGHT] );

				// �r����Ō���
				MoveToEx( hDC, lSkeletonX[K4ABT_JOINT_FOOT_LEFT], lSkeletonY[K4ABT_JOINT_FOOT_LEFT], NULL );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_ANKLE_LEFT], lSkeletonY[K4ABT_JOINT_ANKLE_LEFT] );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_KNEE_LEFT], lSkeletonY[K4ABT_JOINT_KNEE_LEFT] );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_HIP_LEFT], lSkeletonY[K4ABT_JOINT_HIP_LEFT] );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_PELVIS], lSkeletonY[K4ABT_JOINT_PELVIS] );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_HIP_RIGHT], lSkeletonY[K4ABT_JOINT_HIP_RIGHT] );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_KNEE_RIGHT], lSkeletonY[K4ABT_JOINT_KNEE_RIGHT] );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_ANKLE_RIGHT], lSkeletonY[K4ABT_JOINT_ANKLE_RIGHT] );
				LineTo( hDC, lSkeletonX[K4ABT_JOINT_FOOT_RIGHT], lSkeletonY[K4ABT_JOINT_FOOT_RIGHT] );
			}
			// �y�������ɖ߂�
			SelectObject( hDC, hPenPrev );

			EndPaint( hWnd, &ps );
		}
		return 0;
	case WM_CLOSE:
		DestroyWindow( hWnd );
	case WM_DESTROY:
		PostQuitMessage( 0 );
		break;
	default:
		return DefWindowProc( hWnd, uMsg, wParam, lParam );
	}
	return 0;
}

// �A�v���P�[�V�����̏����� (�E�B���h�E��`��p�̃y�����쐬)
HRESULT InitApp( HINSTANCE hInst, int nCmdShow )
{
	WNDCLASSEX wc = { 0, };
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInst;
	wc.hIcon = LoadIcon( NULL, IDI_APPLICATION );
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = (HBRUSH) GetStockObject( NULL_BRUSH );
	wc.lpszClassName = szClassName;
	wc.hIconSm = LoadIcon( NULL, IDI_APPLICATION );
	if ( ! RegisterClassEx( &wc ) )
	{
		MessageBox( NULL, TEXT("�A�v���P�[�V�����N���X�̏������Ɏ��s"), TEXT("�G���["), MB_OK );
		return E_FAIL;
	}

	// �A�v���P�[�V�����E�B���h�E���쐬
	g_hWnd = CreateWindow( szClassName, szClassName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, NULL );
	if ( ! g_hWnd )
	{
		MessageBox( NULL, TEXT("�E�B���h�E�̏������Ɏ��s"), TEXT("�G���["), MB_OK );
		return E_FAIL;
	}

	// �����\���p�̃y�����쐬
	g_hPen = CreatePen( PS_SOLID, 3, RGB( 0, 255, 0 ) );
	if ( ! g_hPen )
	{
		MessageBox( NULL, TEXT("�y���̏������Ɏ��s"), TEXT("�G���["), MB_OK );
		return E_FAIL;
	}

	// ��ʕ\���p�̃r�b�g�}�b�v���쐬����
	ZeroMemory( &g_biBMP, sizeof(g_biBMP) );
	g_biBMP.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	g_biBMP.bmiHeader.biBitCount = 32;
	g_biBMP.bmiHeader.biPlanes = 1;
	g_biBMP.bmiHeader.biWidth = g_iWidth;
	g_biBMP.bmiHeader.biHeight = -(int) g_iHeight;
	g_hBMP = CreateDIBSection( NULL, &g_biBMP, DIB_RGB_COLORS, (LPVOID*) (&g_pdwPixel), NULL, 0 );
	HDC hDC = GetDC( g_hWnd );
	g_hDCBMP = CreateCompatibleDC( hDC );
	ReleaseDC( g_hWnd, hDC );
	g_hBMPold = (HBITMAP) SelectObject( g_hDCBMP, g_hBMP );

	// �[�x�}�b�v�p�o�b�t�@���쐬
	g_pColorMap = new DWORD[g_iWidth * g_iHeight];
	memset( g_pColorMap, 0, g_iWidth * g_iHeight * sizeof(DWORD) );

	// �E�B���h�E��\������
	ShowWindow( g_hWnd, nCmdShow );
	UpdateWindow( g_hWnd );

	return S_OK;
}

// �A�v���P�[�V�����̌�n��
HRESULT UninitApp()
{
	// �J���[�}�b�v���������
	if ( g_pColorMap )
	{
		delete [] g_pColorMap;
		g_pColorMap = NULL;
	}

	// ��ʕ\���p�̃r�b�g�}�b�v���������
	if ( g_hDCBMP || g_hBMP )
	{
		SelectObject( g_hDCBMP, g_hBMPold );
		DeleteObject( g_hBMP );
		DeleteDC( g_hDCBMP );
		g_hBMP = NULL;
		g_hDCBMP = NULL;
	}

	// �����\���p�̃y�����폜
	if ( g_hPen )
	{
		DeleteObject( (HGDIOBJ) g_hPen );
		g_hPen = NULL;
	}
	return S_OK;
}

// �G���g���[�|�C���g
int WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow )
{
	// KINECT ������������
	if ( FAILED( CreateKinect( PLAYBACK_FILENAME ) ) )
		return 1;

	// �A�v���P�[�V����������������
	if ( FAILED( InitApp( hInst, nCmdShow ) ) )
		return 1;

	// �A�v���P�[�V�������[�v
	MSG msg;
	while( GetMessage( &msg, NULL, 0, 0 ) )
	{
		// �E�B���h�E���b�Z�[�W������
		TranslateMessage( &msg );
		DispatchMessage( &msg );

		// Kinect ���ɍX�V������Ε`��
		if ( KinectProc( true ) )
		{
			// �`��
			InvalidateRect( g_hWnd, NULL, TRUE );
		}
	}

	// KINECT ���I������
	DestroyKinect( PLAYBACK_FILENAME );

	// �A�v���P�[�V�������I������
	UninitApp();

	return (int) msg.wParam;
}

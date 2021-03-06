﻿// Copyright (c) Microsoft Corporation
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "pch.h"
#include "Renderer.h"
#include "DDSTextureLoader.h"
#include "Common\DirectXHelper.h"	// For ThrowIfFailed and ReadDataAsync
#include "Game.h"
#include "Utils\PerformanceCounters.h"

using namespace Sample;
using namespace DirectX;
using namespace Windows::Foundation;
using namespace xbox::services::social::manager;

#define COLUMN_1_X                      60
#define COLUMN_2_X                      275
#define COLUMN_3_X                      490
#define ACTION_BUTONS_Y                 60
#if PERF_COUNTERS
#define PERF_X_POS                      900
#define PERF_ROW_OFFSET                 50
#define SOCIAL_GROUP_Y                  400
#define EVENT_LOG_Y                     300
#else
#define EVENT_LOG_Y                     200
#define SOCIAL_GROUP_Y                  300
#endif


extern Game* g_sampleInstance;

Renderer::Renderer(
    const std::shared_ptr<DX::DeviceResources>& deviceResources
    ) :
    m_deviceResources(deviceResources)
{
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}

// Initializes view parameters when the window size changes.
void Renderer::CreateWindowSizeDependentResources()
{
    Size outputSize = m_deviceResources->GetOutputSize();
    float aspectRatio = outputSize.Width / outputSize.Height;
    float fovAngleY = 70.0f * XM_PI / 180.0f;

    // This is a simple example of change that can be made when the app is in
    // portrait or snapped view.
    if (aspectRatio < 1.0f)
    {
        fovAngleY *= 2.0f;
    }

    // Note that the OrientationTransform3D matrix is post-multiplied here
    // in order to correctly orient the scene to match the display orientation.
    // This post-multiplication step is required for any draw calls that are
    // made to the swap chain render target. For draw calls to other targets,
    // this transform should not be applied.

    // This sample makes use of a right-handed coordinate system using row-major matrices.
    XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovRH(
        fovAngleY,
        aspectRatio,
        0.01f,
        100.0f
        );

    XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();

    XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

    m_sprites->SetRotation( m_deviceResources->ComputeDisplayRotation() );
}

void Renderer::Update(DX::StepTimer const& timer)
{
}

void Renderer::Render()
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    // Set render targets to the screen.
    ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
    context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());

    XMVECTORF32 TITLE_COLOR = Colors::Yellow;
    XMVECTORF32 TEXT_COLOR = Colors::White;

    Windows::Foundation::Size screenRect = m_deviceResources->GetLogicalSize();
    FLOAT fWidth = screenRect.Width;
    FLOAT fHeight = screenRect.Height;
    FLOAT fGridX = fWidth / 16.0f;
    FLOAT fGridY = fHeight / 10.0f;
    FLOAT fTextHeight = 25.0f;  // line spacing is double the "x-height".
    FLOAT scale = 0.4f;

    m_sprites->Begin();
    m_font->DrawString(m_sprites.get(), L"Social Manager Sample", XMFLOAT2(COLUMN_1_X * 15.0f, 10), Colors::Yellow, 0.0f, XMFLOAT2(0, 0), 0.5f);

    auto appState = g_sampleInstance->GetGameData()->GetAppState();
    auto gameState = g_sampleInstance->GetGameData()->GetGameState();

    std::vector<XblSocialManagerUserGroup*> socialGroups = g_sampleInstance->GetSocialGroups();
    RenderSocialGroupList(
        COLUMN_1_X,
        COLUMN_2_X,
        COLUMN_3_X,
        SOCIAL_GROUP_Y,
        fTextHeight,
        scale,
        TEXT_COLOR,
        socialGroups
        );

    RenderMenuOptions(scale, TEXT_COLOR);
    RenderEventLog(COLUMN_1_X, EVENT_LOG_Y, fTextHeight, scale, TEXT_COLOR);

#if PERF_COUNTERS
    RenderPerfCounters(PERF_X_POS, PERF_ROW_OFFSET, fTextHeight, scale, TEXT_COLOR);
#endif

    m_sprites->End();
}

void Renderer::CreateDeviceDependentResources()
{
    // Create DirectXTK objects
    auto device = m_deviceResources->GetD3DDevice();
    m_states.reset(new CommonStates(device));

    auto fx = new EffectFactory( device );
    fx->SetDirectory( L"Assets" );
    m_fxFactory.reset( fx );

    auto context = m_deviceResources->GetD3DDeviceContext();
    m_sprites.reset(new SpriteBatch(context));
    m_batch.reset(new PrimitiveBatch<VertexPositionColor>(context));
    m_font.reset(new SpriteFont(device, L"assets\\italic.spritefont"));
}

void Renderer::ReleaseDeviceDependentResources()
{
    m_states.reset();
    m_fxFactory.reset();
    m_sprites.reset();
    m_batch.reset();
    m_font.reset();
}

void Renderer::RenderMenuOptions(
    FLOAT scale, 
    const FXMVECTOR& TEXT_COLOR
    )
{
    WCHAR text[1024];
    swprintf_s(text, ARRAYSIZE(text), L"");

    string_t gamertagString = L"n/a";
    if (g_sampleInstance->GetUser() != nullptr)
    {
        gamertagString = g_sampleInstance->GetGamertag();
    }

    swprintf_s(text, ARRAYSIZE(text),
        L"Press S to sign-in (user signed in: %s).\n"
        L"Press 1 to toggle social group for all friends (%s).\n"
        L"Press 2 to toggle social group for online friends (%s).\n"
        L"Press 3 to toggle social group for all favorites (%s).\n"
        L"Press 4 to toggle social group for online in title (%s).\n"
        L"Press 5 to toggle social group for custom list (%s).\n"
        L"Press C to import custom list.\n"
        L"Press P to get profile\n"
        L"Press F to get friends\n"
        L"Press A to test achievements",
        gamertagString.data(),
        g_sampleInstance->GetAllFriends() ? L"On" : L"Off",
        g_sampleInstance->GetOnlineFriends() ? L"On" : L"Off",
        g_sampleInstance->GetAllFavs() ? L"On" : L"Off",
        g_sampleInstance->GetOnlineInTitle() ? L"On" : L"Off",
        g_sampleInstance->GetCustomList() ? L"On" : L"Off"
        );

    m_font->DrawString(m_sprites.get(), text, XMFLOAT2(COLUMN_1_X, ACTION_BUTONS_Y), TEXT_COLOR, 0.0f, XMFLOAT2(0, 0), scale);
}

void Renderer::RenderEventLog(
    FLOAT fGridXColumn1,
    FLOAT fGridY,
    FLOAT fTextHeight,
    FLOAT scale,
    const FXMVECTOR& TEXT_COLOR
    )
{
    WCHAR text[1024];
    FXMVECTOR TITLE_COLOR = Colors::White;

    fGridY -= 50;

    std::lock_guard<std::mutex> guard(g_sampleInstance->m_displayEventQueueLock);
    if (g_sampleInstance->m_displayEventQueue.size() > 0)
    {
        swprintf_s(text, 128, L"SOCIAL EVENTS:");
        m_font->DrawString(m_sprites.get(), text, XMFLOAT2(15 * fGridXColumn1, fGridY - fTextHeight), Colors::Yellow, 0.0f, XMFLOAT2(0, 0), scale);
    }

    for (unsigned int i = 0; i < g_sampleInstance->m_displayEventQueue.size(); ++i)
    {
        m_font->DrawString(
            m_sprites.get(), 
            g_sampleInstance->m_displayEventQueue[i].c_str(),
            XMFLOAT2(15 * fGridXColumn1, fGridY + (i * fTextHeight) * 1.0f), 
            i >= g_sampleInstance->m_displayEventQueue.size() - g_sampleInstance->m_previousDisplayQueueSize ? TITLE_COLOR : TEXT_COLOR,
            0.0f, XMFLOAT2(0, 0), scale
            );
    }
}

void Renderer::RenderPerfCounters(
    FLOAT fGridXColumn1,
    FLOAT fGridY,
    FLOAT fTextHeight,
    FLOAT scale,
    const XMVECTORF32& TEXT_COLOR
    )
{
    WCHAR text[1024];
    auto perfInstance = performance_counters::get_singleton_instance();
    float verticalBaseOffset = 2 * fTextHeight;

    m_font->DrawString(m_sprites.get(), L"TYPE", XMFLOAT2(fGridXColumn1, fGridY), TEXT_COLOR, 0.0f, XMFLOAT2(0, 0), scale);
    m_font->DrawString(m_sprites.get(), L"AVG", XMFLOAT2(fGridXColumn1 + 150.0f, fGridY), TEXT_COLOR, 0.0f, XMFLOAT2(0, 0), scale);
    m_font->DrawString(m_sprites.get(), L"MIN", XMFLOAT2(fGridXColumn1 + 300.0f, fGridY), TEXT_COLOR, 0.0f, XMFLOAT2(0, 0), scale);
    m_font->DrawString(m_sprites.get(), L"MAX", XMFLOAT2(fGridXColumn1 + 450.0f, fGridY), TEXT_COLOR, 0.0f, XMFLOAT2(0, 0), scale);
    m_font->DrawString(m_sprites.get(), L"_________________________________________________________", XMFLOAT2(fGridXColumn1, fGridY + 10.0f), TEXT_COLOR, 0.0f, XMFLOAT2(0, 0), scale);

    auto noUpdateInstance = perfInstance->get_capture_instace(L"no_updates");
    if (noUpdateInstance != nullptr)
    {
        swprintf_s(text, ARRAYSIZE(text), L"No updates:");
        m_font->DrawString(m_sprites.get(), text, XMFLOAT2(fGridXColumn1, fGridY + verticalBaseOffset), TEXT_COLOR, 0.0f, XMFLOAT2(0, 0), scale);

        swprintf_s(text, ARRAYSIZE(text), L"%s", noUpdateInstance->average_time().ToString()->Data());
        m_font->DrawString(m_sprites.get(), text, XMFLOAT2(fGridXColumn1 + 150.0f, fGridY + verticalBaseOffset), TEXT_COLOR, 0.0f, XMFLOAT2(0, 0), scale);

        swprintf_s(text, ARRAYSIZE(text), L"%s", noUpdateInstance->min_time().ToString()->Data());
        m_font->DrawString(m_sprites.get(), text, XMFLOAT2(fGridXColumn1 + 300.0f, fGridY + verticalBaseOffset), TEXT_COLOR, 0.0f, XMFLOAT2(0, 0), scale);

        swprintf_s(text, ARRAYSIZE(text), L"%s", noUpdateInstance->max_time().ToString()->Data());
        m_font->DrawString(m_sprites.get(), text, XMFLOAT2(fGridXColumn1 + 450.0f, fGridY + verticalBaseOffset), TEXT_COLOR, 0.0f, XMFLOAT2(0, 0), scale);

        verticalBaseOffset += fTextHeight;
    }

    auto updatedInstance = perfInstance->get_capture_instace(L"updates");
    if (updatedInstance != nullptr)
    {
        swprintf_s(text, ARRAYSIZE(text), L"With Updates:");
        m_font->DrawString(m_sprites.get(), text, XMFLOAT2(fGridXColumn1, fGridY + verticalBaseOffset), TEXT_COLOR, 0.0f, XMFLOAT2(0, 0), scale);

        swprintf_s(text, ARRAYSIZE(text), L"%s", updatedInstance->average_time().ToString()->Data());
        m_font->DrawString(m_sprites.get(), text, XMFLOAT2(fGridXColumn1 + 150.0f, fGridY + verticalBaseOffset), TEXT_COLOR, 0.0f, XMFLOAT2(0, 0), scale);

        swprintf_s(text, ARRAYSIZE(text), L"%s", updatedInstance->min_time().ToString()->Data());
        m_font->DrawString(m_sprites.get(), text, XMFLOAT2(fGridXColumn1 + 300.0f, fGridY + verticalBaseOffset), TEXT_COLOR, 0.0f, XMFLOAT2(0, 0), scale);

        swprintf_s(text, ARRAYSIZE(text), L"%s", updatedInstance->max_time().ToString()->Data());
        m_font->DrawString(m_sprites.get(), text, XMFLOAT2(fGridXColumn1 + 450.0f, fGridY + verticalBaseOffset), TEXT_COLOR, 0.0f, XMFLOAT2(0, 0), scale);

        verticalBaseOffset += fTextHeight;
    }
}

std::wstring
ConvertPresenceUserStateToString(
    _In_ XblPresenceUserState presenceState
)
{
    switch (presenceState)
    {
    case XblPresenceUserState_Away: return _T("away");
    case XblPresenceUserState_Offline: return _T("offline");
    case XblPresenceUserState_Online: return _T("online");
    default:
    case XblPresenceUserState_Unknown: return _T("unknown");
    }
}

std::wstring
ConvertPresenceFilterToString(XblPresenceFilter presenceFilter)
{
    switch (presenceFilter)
    {
    case XblPresenceFilter_Unknown: return _T("unknown");
    case XblPresenceFilter_TitleOnline: return _T("title_online");
    case XblPresenceFilter_TitleOffline: return _T("title_offline");
    case XblPresenceFilter_AllOnline: return _T("all_online");
    case XblPresenceFilter_AllOffline: return _T("all_offline");
    case XblPresenceFilter_AllTitle: return _T("all_title");
    default:
    case XblPresenceFilter_All: return _T("all");
    }
}

std::wstring
ConvertRelationshipFilterToString(_In_ XblRelationshipFilter relationshipFilter)
{
    switch (relationshipFilter)
    {
    case XblRelationshipFilter_Favorite: return _T("favorite");
    default:
    case XblRelationshipFilter_Friends: return _T("friends");
    }
}

void
Renderer::RenderSocialGroupList(
    FLOAT fGridXColumn1,
    FLOAT fGridXColumn2,
    FLOAT fGridXColumn3,
    FLOAT fGridY,
    FLOAT fTextHeight,
    FLOAT scale,
    const DirectX::XMVECTORF32& TEXT_COLOR,
    std::vector<XblSocialManagerUserGroup*> nodeList
)
{
    WCHAR text[1024];
    float verticalBaseOffset = 2 * fTextHeight;

    for (auto node : nodeList)
    {
        m_font->DrawString(m_sprites.get(), L"_________________________________________", XMFLOAT2(fGridXColumn1, fGridY + verticalBaseOffset), TEXT_COLOR, 0.0f, XMFLOAT2(0, 0), scale);
        verticalBaseOffset += fTextHeight;
        if (node->socialUserGroupType == XblSocialUserGroupType_FilterType)
        {
            swprintf_s(text, ARRAYSIZE(text), L"Group from filter: %s %s",
                ConvertPresenceFilterToString(node->presenceFilterOfGroup).c_str(),
                ConvertRelationshipFilterToString(node->relationshipFilterOfGroup).c_str()
            );
            m_font->DrawString(m_sprites.get(), text, XMFLOAT2(fGridXColumn1, fGridY + verticalBaseOffset), TEXT_COLOR, 0.0f, XMFLOAT2(0, 0), scale);
            verticalBaseOffset += fTextHeight;
        }
        else
        {
            m_font->DrawString(m_sprites.get(), L"Group from custom list", XMFLOAT2(fGridXColumn1, fGridY + verticalBaseOffset), TEXT_COLOR, 0.0f, XMFLOAT2(0, 0), scale);
            verticalBaseOffset += fTextHeight;
        }

        if (node->usersCount == 0)
        {
            m_font->DrawString(m_sprites.get(), L"No friends found", XMFLOAT2(fGridXColumn1, fGridY + verticalBaseOffset), TEXT_COLOR, 0.0f, XMFLOAT2(0, 0), scale);
            verticalBaseOffset += fTextHeight;
        }
        else
        {
            std::vector<XblSocialManagerUser> userList(node->usersCount);
            XblSocialManagerUserGroupGetUsers(node, userList.data());

            for (const auto& user : userList)
            {
                stringstream_t titleCount;
                titleCount << user.presenceRecord.presenceTitleRecordCount;
                auto titleCountStr = titleCount.str();

                m_font->DrawString(m_sprites.get(), utility::conversions::utf8_to_utf16(user.gamertag).data(), XMFLOAT2(fGridXColumn1, fGridY + verticalBaseOffset), TEXT_COLOR, 0.0f, XMFLOAT2(0, 0), scale);
                m_font->DrawString(m_sprites.get(), ConvertPresenceUserStateToString(user.presenceRecord.userState).c_str(), XMFLOAT2(fGridXColumn2, fGridY + verticalBaseOffset), TEXT_COLOR, 0.0f, XMFLOAT2(0, 0), scale);
                m_font->DrawString(m_sprites.get(), titleCountStr.c_str(), XMFLOAT2(fGridXColumn3, fGridY + verticalBaseOffset), TEXT_COLOR, 0.0f, XMFLOAT2(0, 0), scale);

                verticalBaseOffset += fTextHeight;
            }
        }
    }
}
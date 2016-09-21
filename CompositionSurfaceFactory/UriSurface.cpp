#include "pch.h"
#include "SurfaceFactory.h"
#include "UriSurface.h"
#include "SharedLock.h"

using namespace Robmikh::CompositionSurfaceFactory;
using namespace Platform;

namespace CSF = Robmikh::CompositionSurfaceFactory;

UriSurface^ UriSurface::Create(
    CSF::SurfaceFactory^ surfaceFactory,
    Uri^ uri,
    WF::Size size,
    CSF::InterpolationMode interpolation)
{
    return ref new UriSurface(surfaceFactory, uri, size, interpolation);
}

UriSurface::UriSurface(
    CSF::SurfaceFactory^ surfaceFactory,
    Uri^ uri, 
    WF::Size size,
    CSF::InterpolationMode interpolation) : 
    m_surfaceFactory(surfaceFactory), 
    m_uri(uri),
    m_interpolationMode(interpolation),
    m_desiredSize(size)
{
    m_surface = m_surfaceFactory->CreateSurface(m_desiredSize);

    OnDeviceReplacedHandler = 
        m_surfaceFactory->DeviceReplaced += 
            ref new EventHandler<RenderingDeviceReplacedEventArgs ^>(this, &UriSurface::OnDeviceReplacedEvent);
}

UriSurface::~UriSurface()
{
    Uninitialize();
}

void UriSurface::OnDeviceReplacedEvent(Object ^sender, RenderingDeviceReplacedEventArgs ^args)
{
    OutputDebugString(L"CompositionSurfaceFactory - Redrawing UriSurface from Device Replaced");
    auto ignored = RedrawSurface();
}

IAsyncAction^ UriSurface::RedrawSurface()
{
    return RedrawSurface(m_uri);
}

IAsyncAction^ UriSurface::RedrawSurface(Uri^ uri)
{
    return RedrawSurface(uri, m_desiredSize);
}

IAsyncAction^ UriSurface::RedrawSurface(Uri^ uri, WF::Size size)
{
    return RedrawSurface(uri, size, m_interpolationMode);
}

IAsyncAction^ UriSurface::RedrawSurface(Uri^ uri, WF::Size size, CSF::InterpolationMode interpolation)
{
    m_desiredSize = size;
    return concurrency::create_async([this, uri, size, interpolation]() -> concurrency::task<void>
    {
        return RedrawSurfaceWorker(uri, size, interpolation);
    });
}

void UriSurface::Resize(WF::Size size)
{
    auto lock = m_surfaceFactory->DrawingLock;

    lock->Lock(ref new SharedLockWork([=]() mutable
    {
        CanvasComposition::Resize(m_surface, size);
    }));
}

concurrency::task<void> UriSurface::RedrawSurfaceWorker(Uri^ uri, WF::Size size, CSF::InterpolationMode interpolation) __resumable
{
    m_uri = uri;
    m_interpolationMode = interpolation;
    co_await m_surfaceFactory->DrawSurface(m_surface, m_uri, size, m_interpolationMode);
}

void UriSurface::Uninitialize()
{
    m_surfaceFactory->DeviceReplaced -= OnDeviceReplacedHandler;
    m_surface->~CompositionDrawingSurface();
    m_surface = nullptr;
    m_surfaceFactory = nullptr;
    m_uri = nullptr;
}
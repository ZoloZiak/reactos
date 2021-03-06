/*
 * IDirect3DSurface8 implementation
 *
 * Copyright 2005 Oliver Stieber
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "d3d8_private.h"

static inline struct d3d8_surface *impl_from_IDirect3DSurface8(IDirect3DSurface8 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d8_surface, IDirect3DSurface8_iface);
}

static HRESULT WINAPI d3d8_surface_QueryInterface(IDirect3DSurface8 *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DSurface8)
            || IsEqualGUID(riid, &IID_IDirect3DResource8)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3DSurface8_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d8_surface_AddRef(IDirect3DSurface8 *iface)
{
    struct d3d8_surface *surface = impl_from_IDirect3DSurface8(iface);

    TRACE("iface %p.\n", iface);

    if (surface->forwardReference)
    {
        /* Forward refcounting */
        TRACE("Forwarding to %p.\n", surface->forwardReference);
        return IUnknown_AddRef(surface->forwardReference);
    }
    else
    {
        /* No container, handle our own refcounting */
        ULONG ref = InterlockedIncrement(&surface->resource.refcount);

        TRACE("%p increasing refcount to %u.\n", iface, ref);

        if (ref == 1)
        {
            if (surface->parent_device)
                IDirect3DDevice8_AddRef(surface->parent_device);
            wined3d_mutex_lock();
            wined3d_surface_incref(surface->wined3d_surface);
            wined3d_mutex_unlock();
        }

        return ref;
    }
}

static ULONG WINAPI d3d8_surface_Release(IDirect3DSurface8 *iface)
{
    struct d3d8_surface *surface = impl_from_IDirect3DSurface8(iface);

    TRACE("iface %p.\n", iface);

    if (surface->forwardReference)
    {
        /* Forward refcounting */
        TRACE("Forwarding to %p.\n", surface->forwardReference);
        return IUnknown_Release(surface->forwardReference);
    }
    else
    {
        /* No container, handle our own refcounting */
        ULONG ref = InterlockedDecrement(&surface->resource.refcount);

        TRACE("%p decreasing refcount to %u.\n", iface, ref);

        if (!ref)
        {
            IDirect3DDevice8 *parent_device = surface->parent_device;

            /* Implicit surfaces are destroyed with the device, not if refcount reaches 0. */
            wined3d_mutex_lock();
            wined3d_surface_decref(surface->wined3d_surface);
            wined3d_mutex_unlock();

            if (parent_device)
                IDirect3DDevice8_Release(parent_device);
        }

        return ref;
    }
}

static HRESULT WINAPI d3d8_surface_GetDevice(IDirect3DSurface8 *iface, IDirect3DDevice8 **device)
{
    struct d3d8_surface *surface = impl_from_IDirect3DSurface8(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    if (surface->forwardReference)
    {
        IDirect3DResource8 *resource;
        HRESULT hr;

        hr = IUnknown_QueryInterface(surface->forwardReference, &IID_IDirect3DResource8, (void **)&resource);
        if (SUCCEEDED(hr))
        {
            hr = IDirect3DResource8_GetDevice(resource, device);
            IDirect3DResource8_Release(resource);

            TRACE("Returning device %p.\n", *device);
        }

        return hr;
    }

    *device = surface->parent_device;
    IDirect3DDevice8_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI d3d8_surface_SetPrivateData(IDirect3DSurface8 *iface, REFGUID guid,
        const void *data, DWORD data_size, DWORD flags)
{
    struct d3d8_surface *surface = impl_from_IDirect3DSurface8(iface);
    TRACE("iface %p, guid %s, data %p, data_size %u, flags %#x.\n",
            iface, debugstr_guid(guid), data, data_size, flags);

    return d3d8_resource_set_private_data(&surface->resource, guid, data, data_size, flags);
}

static HRESULT WINAPI d3d8_surface_GetPrivateData(IDirect3DSurface8 *iface, REFGUID guid,
        void *data, DWORD *data_size)
{
    struct d3d8_surface *surface = impl_from_IDirect3DSurface8(iface);
    TRACE("iface %p, guid %s, data %p, data_size %p.\n",
            iface, debugstr_guid(guid), data, data_size);

    return d3d8_resource_get_private_data(&surface->resource, guid, data, data_size);
}

static HRESULT WINAPI d3d8_surface_FreePrivateData(IDirect3DSurface8 *iface, REFGUID guid)
{
    struct d3d8_surface *surface = impl_from_IDirect3DSurface8(iface);
    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(guid));

    return d3d8_resource_free_private_data(&surface->resource, guid);
}

static HRESULT WINAPI d3d8_surface_GetContainer(IDirect3DSurface8 *iface, REFIID riid, void **container)
{
    struct d3d8_surface *surface = impl_from_IDirect3DSurface8(iface);
    HRESULT hr;

    TRACE("iface %p, riid %s, container %p.\n", iface, debugstr_guid(riid), container);

    if (!surface->container)
        return E_NOINTERFACE;

    hr = IUnknown_QueryInterface(surface->container, riid, container);

    TRACE("Returning %p.\n", *container);

    return hr;
}

static HRESULT WINAPI d3d8_surface_GetDesc(IDirect3DSurface8 *iface, D3DSURFACE_DESC *desc)
{
    struct d3d8_surface *surface = impl_from_IDirect3DSurface8(iface);
    struct wined3d_resource_desc wined3d_desc;
    struct wined3d_resource *wined3d_resource;

    TRACE("iface %p, desc %p.\n", iface, desc);

    wined3d_mutex_lock();
    wined3d_resource = wined3d_surface_get_resource(surface->wined3d_surface);
    wined3d_resource_get_desc(wined3d_resource, &wined3d_desc);
    wined3d_mutex_unlock();

    desc->Format = d3dformat_from_wined3dformat(wined3d_desc.format);
    desc->Type = wined3d_desc.resource_type;
    desc->Usage = wined3d_desc.usage & WINED3DUSAGE_MASK;
    desc->Pool = wined3d_desc.pool;
    desc->Size = wined3d_desc.size;
    desc->MultiSampleType = wined3d_desc.multisample_type;
    desc->Width = wined3d_desc.width;
    desc->Height = wined3d_desc.height;

    return D3D_OK;
}

static HRESULT WINAPI d3d8_surface_LockRect(IDirect3DSurface8 *iface,
        D3DLOCKED_RECT *locked_rect, const RECT *rect, DWORD flags)
{
    struct d3d8_surface *surface = impl_from_IDirect3DSurface8(iface);
    struct wined3d_map_desc map_desc;
    HRESULT hr;

    TRACE("iface %p, locked_rect %p, rect %s, flags %#x.\n",
            iface, locked_rect, wine_dbgstr_rect(rect), flags);

    wined3d_mutex_lock();
    if (rect)
    {
        D3DSURFACE_DESC desc;
        IDirect3DSurface8_GetDesc(iface, &desc);

        if ((rect->left < 0)
                || (rect->top < 0)
                || (rect->left >= rect->right)
                || (rect->top >= rect->bottom)
                || (rect->right > desc.Width)
                || (rect->bottom > desc.Height))
        {
            WARN("Trying to lock an invalid rectangle, returning D3DERR_INVALIDCALL\n");
            wined3d_mutex_unlock();

            return D3DERR_INVALIDCALL;
        }
    }

    hr = wined3d_surface_map(surface->wined3d_surface, &map_desc, rect, flags);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr))
    {
        locked_rect->Pitch = map_desc.row_pitch;
        locked_rect->pBits = map_desc.data;
    }
    else
    {
        locked_rect->Pitch = 0;
        locked_rect->pBits = NULL;
    }

    return hr;
}

static HRESULT WINAPI d3d8_surface_UnlockRect(IDirect3DSurface8 *iface)
{
    struct d3d8_surface *surface = impl_from_IDirect3DSurface8(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = wined3d_surface_unmap(surface->wined3d_surface);
    wined3d_mutex_unlock();

    switch(hr)
    {
        case WINEDDERR_NOTLOCKED:       return D3DERR_INVALIDCALL;
        default:                        return hr;
    }
}

static const IDirect3DSurface8Vtbl d3d8_surface_vtbl =
{
    /* IUnknown */
    d3d8_surface_QueryInterface,
    d3d8_surface_AddRef,
    d3d8_surface_Release,
    /* IDirect3DResource8 */
    d3d8_surface_GetDevice,
    d3d8_surface_SetPrivateData,
    d3d8_surface_GetPrivateData,
    d3d8_surface_FreePrivateData,
    /* IDirect3DSurface8 */
    d3d8_surface_GetContainer,
    d3d8_surface_GetDesc,
    d3d8_surface_LockRect,
    d3d8_surface_UnlockRect,
};

static void STDMETHODCALLTYPE surface_wined3d_object_destroyed(void *parent)
{
    struct d3d8_surface *surface = parent;
    d3d8_resource_cleanup(&surface->resource);
    HeapFree(GetProcessHeap(), 0, surface);
}

static const struct wined3d_parent_ops d3d8_surface_wined3d_parent_ops =
{
    surface_wined3d_object_destroyed,
};

void surface_init(struct d3d8_surface *surface, struct wined3d_surface *wined3d_surface,
        struct d3d8_device *device, const struct wined3d_parent_ops **parent_ops)
{
    surface->IDirect3DSurface8_iface.lpVtbl = &d3d8_surface_vtbl;
    d3d8_resource_init(&surface->resource);
    wined3d_surface_incref(wined3d_surface);
    surface->wined3d_surface = wined3d_surface;
    surface->parent_device = &device->IDirect3DDevice8_iface;
    IDirect3DDevice8_AddRef(surface->parent_device);

    *parent_ops = &d3d8_surface_wined3d_parent_ops;
}

struct d3d8_surface *unsafe_impl_from_IDirect3DSurface8(IDirect3DSurface8 *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d8_surface_vtbl);

    return impl_from_IDirect3DSurface8(iface);
}

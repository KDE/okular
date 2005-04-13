//----------------------------------------------------------------------------
// Anti-Grain Geometry - Version 2.3
// Copyright (C) 2002-2005 Maxim Shemanarev (http://www.antigrain.com)
//
// Permission to copy, use, modify, sell and distribute this software 
// is granted provided this copyright notice appears in all copies. 
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//
//----------------------------------------------------------------------------
// Contact: mcseem@antigrain.com
//          mcseemagg@yahoo.com
//          http://www.antigrain.com
//----------------------------------------------------------------------------
//
// Adaptation for high precision colors has been sponsored by 
// Liberty Technology Systems, Inc., visit http://lib-sys.com
//
// Liberty Technology Systems, Inc. is the provider of
// PostScript and PDF technology for software developers.
// 
//----------------------------------------------------------------------------

/**
 * @short KPDF modified rgba pixel format provider for rastering operations
 * This is the agg_pixfmt_rgba.h file with some added code. When syncing to
 * the latest agg counterpart, diff the mods and apply them again after
 * copying the legacy file over this.
 * - Enrico Ros @ KDPF Team - 2005
 *
 * Thanks to Rob Buis <rwlbuis@xs4all.nl> for providing the raster algorithms
 * he used when modifying agg for usage with KCanvas/KSvg2.
 */

#ifndef AGG_PIXFMT_RGBA_INCLUDED
#define AGG_PIXFMT_RGBA_INCLUDED

#include <string.h>
#include "agg_basics.h"
#include "agg_color_rgba.h"
#include "agg_rendering_buffer.h"

namespace agg
{
    // Nice trick from Qt4's Arthur
    inline int qt_div_255(int x) { return (x + (x>>8) + 0x80) >> 8; }

    //=============================================================normal_blend_rgba
    /** The standard blending algo */
    template<class ColorT, class Order, class PixelT> struct normal_blend_rgba
    {
        typedef ColorT color_type;
        typedef PixelT pixel_type;
        typedef Order order_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum
        {
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        //--------------------------------------------------------------------
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned cr, unsigned cg, unsigned cb,
                                         unsigned alpha, 
                                         unsigned)
        {
            calc_type r = p[Order::R];
            calc_type g = p[Order::G];
            calc_type b = p[Order::B];
            calc_type a = p[Order::A];
            p[Order::R] = (value_type)(((cr - r) * alpha + (r << base_shift)) >> base_shift);
            p[Order::G] = (value_type)(((cg - g) * alpha + (g << base_shift)) >> base_shift);
            p[Order::B] = (value_type)(((cb - b) * alpha + (b << base_shift)) >> base_shift);
            p[Order::A] = (value_type)((alpha + a) - ((alpha * a + base_mask) >> base_shift));
        }
    };

    //=============================================================multiply_blend_rgba
    /** The Multiplier blending algo (for Highlighting pigmentation) */
    template<class ColorT, class Order, class PixelT> struct multiply_blend_rgba
    {
        typedef ColorT color_type;
        typedef PixelT pixel_type;
        typedef Order order_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum
        {
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        //--------------------------------------------------------------------
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned cr, unsigned cg, unsigned cb,
                                         unsigned alpha, 
                                         unsigned)
        {
            calc_type r = p[Order::R];
            calc_type g = p[Order::G];
            calc_type b = p[Order::B];
            calc_type a = p[Order::A];
            p[Order::R] = (value_type)(((qt_div_255(cr*r) - r) * alpha + (r << base_shift)) >> base_shift);
            p[Order::G] = (value_type)(((qt_div_255(cg*g) - g) * alpha + (g << base_shift)) >> base_shift);
            p[Order::B] = (value_type)(((qt_div_255(cb*b) - b) * alpha + (b << base_shift)) >> base_shift);
            p[Order::A] = (value_type)((alpha + a) - ((alpha * a + base_mask) >> base_shift));
        }
    };

    //=======================================================pixel_formats_rgba
    /** The following functions have been modified for using different blenders:
     *   blend_pixel, blend_hline, blend_solid_hspan
     * (the others are not used by kpdf and have been removed)
     */
    template<class ColorT, class Order, class PixelT> class pixel_formats_rgba
    {
    public:
        typedef rendering_buffer::row_data row_data;
        typedef ColorT color_type;
        typedef Order order_type;
        typedef PixelT pixel_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum
        {
            base_shift = color_type::base_shift,
            base_size  = color_type::base_size,
            base_mask  = color_type::base_mask
        };
        typedef normal_blend_rgba<ColorT, Order, PixelT> normal_blender;
        typedef multiply_blend_rgba<ColorT, Order, PixelT> multiply_blender;

        //--------------------------------------------------------------------
        pixel_formats_rgba( rendering_buffer& rb, int rasterMode ) :
            m_rbuf(&rb), m_mode( rasterMode )
        {}

        //--------------------------------------------------------------------
        AGG_INLINE unsigned width()  const { return m_rbuf->width();  }
        AGG_INLINE unsigned height() const { return m_rbuf->height(); }

        //--------------------------------------------------------------------
        AGG_INLINE void blend_pixel(int x, int y, const color_type& c, int8u cover)
        {
            if ( !c.a )
                return;

            value_type* p = (value_type*)m_rbuf->row(y) + (x << 2);

            if ( !m_mode )
            {
                calc_type alpha = (calc_type(c.a) * (cover + 1)) >> 8;
                if(alpha == base_mask)
                {
                    p[order_type::R] = c.r;
                    p[order_type::G] = c.g;
                    p[order_type::B] = c.b;
                    p[order_type::A] = c.a;
                }
                else
                    normal_blender::blend_pix(p, c.r, c.g, c.b, alpha, cover);
            }
            else if ( m_mode == 1 )
                multiply_blender::blend_pix(p, c.r, c.g, c.b, alpha, cover);
        }

        //--------------------------------------------------------------------
        void blend_hline(int x, int y,
                         unsigned len, 
                         const color_type& c,
                         int8u cover)
        {
            if ( !c.a )
                return;

            value_type* p = (value_type*)m_rbuf->row(y) + (x << 2);
            calc_type alpha = (calc_type(c.a) * (cover + 1)) >> 8;
            switch ( m_mode )
            {
                case 0:
                    if ( alpha == base_mask )
                    {
                        pixel_type v;
                        ((value_type*)&v)[order_type::R] = c.r;
                        ((value_type*)&v)[order_type::G] = c.g;
                        ((value_type*)&v)[order_type::B] = c.b;
                        ((value_type*)&v)[order_type::A] = c.a;
                        while( len-- )
                        {
                            *(pixel_type*)p = v;
                            p += 4;
                        }
                    }
                    else
                    {
                        while( len-- )
                        {
                            normal_blender::blend_pix(p, c.r, c.g, c.b, alpha, cover);
                            p += 4;
                        }
                    }
                    break;
                case 1:
                    while( len-- )
                    {
                        multiply_blender::blend_pix(p, c.r, c.g, c.b, alpha, cover);
                        p += 4;
                    }
                    break;
            }
        }

        //--------------------------------------------------------------------
        void blend_solid_hspan(int x, int y,
                               unsigned len, 
                               const color_type& c,
                               const int8u* covers)
        {
            if ( !c.a )
                return;

            value_type* p = (value_type*)m_rbuf->row(y) + (x << 2);
            switch ( m_mode )
            {
                case 0:
                    while( len-- )
                    {
                        calc_type alpha = (calc_type(c.a) * (calc_type(*covers) + 1)) >> 8;
                        if ( alpha == base_mask )
                        {
                            p[order_type::R] = c.r;
                            p[order_type::G] = c.g;
                            p[order_type::B] = c.b;
                            p[order_type::A] = base_mask;
                        }
                        else
                            normal_blender::blend_pix(p, c.r, c.g, c.b, alpha, *covers);
                        p += 4;
                        ++covers;
                    }
                    break;
                case 1:
                    while( len-- )
                    {
                        calc_type alpha = (calc_type(c.a) * (calc_type(*covers) + 1)) >> 8;
                        multiply_blender::blend_pix(p, c.r, c.g, c.b, alpha, *covers);
                        p += 4;
                        ++covers;
                    }
                    break;
            }
        }

    private:
        rendering_buffer* m_rbuf;
        int m_mode;
    };

    //-----------------------------------------------------------------------
    // Order Types: order_rgba, order_argb, order_abgr, [order_bgra]
    // the RBGA32 is used by kpdf/qimage ( 0xAARRGGBB in memory )
    typedef pixel_formats_rgba<rgba8, order_bgra, int32u> pixfmt_bgra32;
}

#endif

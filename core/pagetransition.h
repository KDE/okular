/***************************************************************************
 *   Copyright (C) 2005 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_PAGE_TRANSITION_H_
#define _KPDF_PAGE_TRANSITION_H_

/**
 * @short Information object for the transition effect of a page.
 */
class KPDFPageTransition
{
    public:
        enum Type {
            Replace,
            Split,
            Blinds,
            Box,
            Wipe,
            Dissolve,
            Glitter,
            Fly,
            Push,
            Cover,
            Uncover,
            Fade
        };

        enum Alignment {
            Horizontal,
            Vertical
        };

        enum Direction {
            Inward,
            Outward
        };

        KPDFPageTransition( Type type = Replace );
        ~KPDFPageTransition();

        // Get type of the transition.
        inline Type type() const { return m_type; }

        // Get duration of the transition in seconds.
        inline int duration() const { return m_duration; }

        // Get dimension in which the transition effect occurs.
        inline Alignment alignment() const { return m_alignment; }

        // Get direction of motion of the transition effect.
        inline Direction direction() const { return m_direction; }

        // Get direction in which the transition effect moves.
        inline int angle() const { return m_angle; }

        // Get starting or ending scale. (Fly only)
        inline double scale() const { return m_scale; }

        // Returns true if the area to be flown is rectangular and opaque.  (Fly only)
        inline bool isRectangular() const { return m_rectangular; }

        inline void setType( Type type ) { m_type = type; }
        inline void setDuration( int duration ) { m_duration = duration; }
        inline void setAlignment( Alignment alignment ) { m_alignment = alignment; }
        inline void setDirection( Direction direction ) { m_direction = direction; }
        inline void setAngle( int angle ) { m_angle = angle; }
        inline void setScale( double scale ) { m_scale = scale; }
        inline void setIsRectangular( bool rectangular ) { m_rectangular = rectangular; }

    private:
        Type m_type;
        int m_duration;
        Alignment m_alignment;
        Direction m_direction;
        int m_angle;
        double m_scale;
        bool m_rectangular;
};

#endif

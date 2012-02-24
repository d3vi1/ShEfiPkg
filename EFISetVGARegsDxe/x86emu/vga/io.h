/*
 * io.h:
 *
 * Copyright (c) 2006 James McKenzie <james@fishsoup.dhs.org>,
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*
 * $Id: io.h,v 1.1 2006/05/27 18:41:14 root Exp $
 */

/*
 * $Log: io.h,v $
 * Revision 1.1  2006/05/27 18:41:14  root
 * *** empty log message ***
 *
 * Revision 1.1  2006/05/22 06:44:42  root
 * *** empty log message ***
 *
 * Revision 1.1  2006/05/02 19:22:56  james
 * *** empty log message ***
 *
 */

#ifndef __IO_H__
#define __IO_H__


/* IO port input/output */
#if 0

/* XXX we should be able to use the EFI functions for these, but they don't
 * link under gnu-efi. */
#define out_8(val, port)        outp((port), (val))
#define out_16(val, port)       outpw((port), (val))
#define out_32(val, port)       outpd((port), (val))

#define in_8(port)              inp((port))
#define in_16(port)             inpw((port))
#define in_32(port)             inpd((port))

#endif

static inline void out_8(uint8_t value, uint16_t port) {
    __asm__ __volatile__ ("out" "b" " %" "b" "0,%" "w" "1"::"a" (value),
            "Nd" (port));
}

static inline void out_16(uint16_t value, uint16_t port) {
    __asm__ __volatile__ ("out" "w" " %" "w" "0,%" "w" "1"::"a" (value),
            "Nd" (port));
}

static inline void out_32(uint32_t value, uint16_t port) {
    __asm__ __volatile__ ("out" "l" " %" "0,%" "w" "1"::"a" (value),
            "Nd" (port));
}

static inline uint8_t in_8(uint16_t port) {
    uint8_t _v;
    __asm__ __volatile__ ("in" "b" " %" "w" "1,%" "" "0":"=a" (_v):"Nd" (port));
    return _v;
}

static inline uint16_t in_16(uint16_t port) {
    uint16_t _v;
    __asm__ __volatile__ ("in" "w" " %" "w" "1,%" "" "0":"=a" (_v):"Nd" (port));
    return _v;
}

static inline uint32_t in_32(uint16_t port) {
    uint32_t _v;
    __asm__ __volatile__ ("in" "l" " %" "w" "1,%" "" "0":"=a" (_v):"Nd" (port));
    return _v;
}

#endif /* __IO_H__ */

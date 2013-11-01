/***************************************************************************
 *   Copyright (C) 2007 by Arep                                            *
 *   Support is provided through the forums at                             *
 *   http://wii.console-tribe.com                                          *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/*! \file
 * \brief Nintendo GameCube/Wii disc geometry constants.
 *
 * This file contains constants that describe the general layout of Nintendo GameCube/Wii discs and that can be used throughout the whole program.
 */

#ifndef CONSTANTS_H_INCLUDED
#define CONSTANTS_H_INCLUDED

/*! \brief Size of a scrambled sector */
#define RAW_SECTOR_SIZE 2064

/*! \brief Size of an unscrambled sector */
#define SECTOR_SIZE 2048

/*! \brief Number of sectors in a block */
#define SECTORS_PER_BLOCK 16

/*! \brief Size of a scrambled block */
#define RAW_BLOCK_SIZE (RAW_SECTOR_SIZE * SECTORS_PER_BLOCK)

/*! \brief Size of an unscrambled block */
#define BLOCK_SIZE (SECTOR_SIZE * SECTORS_PER_BLOCK)

#endif

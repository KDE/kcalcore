/*
    This file is part of the kcal library.

     Copyright (C) 2004 Dirk Mueller <mueller@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
#ifndef KCAL_H
#define KCAL_H

#include <kdemacros.h>

#if defined(_WIN32) || defined(_WIN64)
#ifdef MAKE_KCAL_LIB
#define KCAL_EXPORT KDE_EXPORT
#else
#define KCAL_EXPORT KDE_IMPORT
#endif
#else
#define KCAL_EXPORT KDE_EXPORT
#endif

#ifdef COMPILING_TESTS
#if defined(_WIN32) || defined(_WIN64)
#ifdef MAKE_KCAL_LIB
#define KCAL_TEST_EXPORT KDE_EXPORT
#else
#define KCAL_TEST_EXPORT KDE_IMPORT
#endif
#else
#define KCAL_TEST_EXPORT KDE_EXPORT
#endif
#else /*no unit tests compiled*/
#define KCAL_TEST_EXPORT
#endif /* COMPILING_TESTS */

/**
  @namespace KCal

  @brief
  Contains all the KCal library global classes, objects, and functions.
*/

#endif

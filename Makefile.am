################################################################################
#
# \file Makefile.am
# \brief Global makefile for the SU2 project
# \author M. Colonno, T. Economon, F. Palacios
# \version 7.2.1 "Blackbird"
#
# The current SU2 release has been coordinated by the
# SU2 International Developers Society <www.su2devsociety.org>
# with selected contributions from the open-source community.
#
# The main research teams contributing to the current release are:
#  - Prof. Juan J. Alonso's group at Stanford University.
#  - Prof. Piero Colonna's group at Delft University of Technology.
#  - Prof. Nicolas R. Gauger's group at Kaiserslautern University of Technology.
#  - Prof. Alberto Guardone's group at Polytechnic University of Milan.
#  - Prof. Rafael Palacios' group at Imperial College London.
#  - Prof. Vincent Terrapon's group at the University of Liege.
#  - Prof. Edwin van der Weide's group at the University of Twente.
#  - Lab. of New Concepts in Aeronautics at Tech. Institute of Aeronautics.
#
# Copyright 2012-2021, Francisco D. Palacios, Thomas D. Economon,
#                      Tim Albring, and the SU2 contributors.
#
# SU2 is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# SU2 is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with SU2. If not, see <http://www.gnu.org/licenses/>.
#
################################################################################

AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

# build third-party optional dependencies first
SUBDIRS = externals

SUBDIRS += Common/lib

if BUILD_PY
SUBDIRS += SU2_PY
endif

if BUILD_CFD
SUBDIRS +=SU2_CFD/obj
endif

if BUILD_DOT
SUBDIRS +=SU2_DOT/obj
endif

if BUILD_DEF
SUBDIRS +=SU2_DEF/obj
endif

if BUILD_SOL
SUBDIRS +=SU2_SOL/obj
endif

if BUILD_GEO
SUBDIRS +=SU2_GEO/obj
endif

if BUILD_PY_WRAPPER
SUBDIRS += SU2_PY/pySU2
endif

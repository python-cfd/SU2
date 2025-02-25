################################################################################
#
# \file Makefile.am
# \brief Makefile for SU2_GEO
# \author M. Colonno, T. Economon, F. Palacios
# \version 7.2.1 "Blackbird"
#
# SU2 Project Website: https://su2code.github.io
# 
# The SU2 Project is maintained by the SU2 Foundation 
# (http://su2foundation.org)
#
# Copyright 2012-2021, SU2 Contributors (cf. AUTHORS.md)
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
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

bin_PROGRAMS = ../bin/SU2_GEO

___bin_SU2_GEO_SOURCES = \
  ../include/SU2_GEO.hpp \
  ../src/SU2_GEO.cpp

___bin_SU2_GEO_CXXFLAGS =
___bin_SU2_GEO_LDADD = ../../Common/lib/libSU2.a

# always link to built dependencies from ./externals
___bin_SU2_GEO_CXXFLAGS += @su2_externals_INCLUDES@
___bin_SU2_GEO_LDADD += @su2_externals_LIBS@

# if BUILD_MUTATIONPP
___bin_SU2_GEO_CXXFLAGS += @MUTATIONPP_CXX@
___bin_SU2_GEO_LDADD += @MUTATIONPP_LD@
# endif

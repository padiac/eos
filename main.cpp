/*
  -------------------------------------------------------------------
  
  Copyright (C) 2018-2019, Xingfu Du and Andrew W. Steiner
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.

  -------------------------------------------------------------------
*/
#include "eos.h"

using namespace std;
using namespace o2scl;

int main(int argc, char *argv[]) {

  cout.setf(ios::scientific);

  eos eph;
  cli cl;
  
  eph.setup_cli(cl);

  cl.run_auto(argc,argv);
  
  return 0;
}

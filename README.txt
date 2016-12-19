*****Compilation 
Instructions below are a bit obsolete and APE is moved to offline svn area. Athena based instructions are rather up-to-date

* Dependecies 

APE framework depends on intel TBB4.0+ and yampl and
requires a compiler supporting c++11. You can get yampl from
https://github.com/vitillo/yampl.git and TBB from
https://www.threadingbuildingblocks.org/. See below Quick instructions
for details

*Quick instructions (for standalone, see below for Athena)

Below are some instructions to build the package. APEDIR(or ${APEDIR}
corresponds to work directory, TBB_INST_DIR corresponds to directory
where the TBB is installed. Please replace them with correct values in
your work environment.

cd ${APEDIR}
git clone https://github.com/vitillo/yampl.git
git clone /afs/cern.ch/work/k/kama/GitRepositories/APERelease/public/APE
cd yampl
./configure --prefix="${PWD}/../InstallArea/"
make clean && make && make install
cd ${APEDIR}
source <TBB_INST_DIR>/tbbvars.sh
export LD_LIBRARY_PATH=${APEDIR}/InstallArea/lib:${LD_LIBRARY_PATH}
mkdir BuildArea
cd BuildArea
ccmake28 ../APE (or cmake28-gui)
* enable BUILD_EXAMPLES 
* set CMAKE_CXX_COMPILER and CMAKE_C_COMPILER to c++11 supporting compiler suite (i.e. gcc4.7.2+, icc14, llvm3.4 etc) 
* set CMAKE_INSTALL_PREFIX to <APEDIR>/InstallArea/
* configure, generate makefiles and exit

(or cmake28 -DCMAKE_CXX_COMPILER="$(which g++)" -DCMAKE_C_COMPILER="$(which gcc)" -DCMAKE_INSTALL_PREFIX=${APEDIR}/InstallArea/ )
make && make install

This will build and install apeMod, dctClient and module for dct
example, libdctModule.so. To run the example run two shells/terminals
and setup the environment(i.e. PATH and LD_LIBRARY_PATH for
cuda,compiler,tbb, yampl and APE installation area). In one shell
execute

dctClient -d $APEDIR/APE/examples/dct8x8/data -n 1000

for 1000 transfers between client and server.
In another execute

apeMod -m libdctModule.so

When both processes are started, client reads data files in the
directory and sends them to the server to be processed and receives
processed work back.

**  Quick instructions (for Athena based environment)
setup athena (brings in yampl, tbb and gcc4x)
get APE and APEModules to your work directory

cmt co Offloading/APE
cmt co Offloading/APEModules
mkdir BuildArea

run ccmake28 ../Offloading/APE or cmake28-gui (you can use /afs/cern.ch/sw/lcg/contrib/CMake/2.8.12.2/Linux-i386/bin/cmake-gui if you have afs)

* enable BUILD_EXAMPLES 
* set CMAKE_CXX_COMPILER and CMAKE_C_COMPILER to c++11 supporting compiler suite (i.e. gcc4.7.2+, icc14, llvm3.4 etc) 
* set CMAKE_INSTALL_PREFIX to where you want to install
* set APEModuleDir to Offloading/APEModules (where you checked out APEModules)
* configure, generate makefiles and exit
make (and make install if you want to install)

** Installation Requirements and Building

In order to be able to compile the framework you need to setup TBB
environment and add the directory where yampl library is installed to
your LD_LIBRARY_PATH. CMake macro to find yampl package assumes yampl
headers are installed in ../include/ with respect to the directory
where the library is installed. This is the default behavior for
automake packages unless you changed the installation paths of headers
and libraries individually by passing flags to configure script. 

yampl depends on libuuid. For SLC6 uuid-devel-1.6.1-10 package will
install the headers necessary for libuuid. However you can have use
the package from
http://sourceforge.net/projects/libuuid/files/latest/download to build
libuuid for different architectures

dct8x8 example needs cuda 6.0 to be present in the system. However
this can be changed to older cuda releases by modifying the
CMakeLists.txt file for the example

* Building
Default usage flags doesn't enable compilation of modules or
examples. If BUILD_MODULES flags is enabled, CMAKE will try to include
the directory set in APE_MODULES_DIR as a subdirectory to top level
cmake file.



**** A little more details

This is the server part of the APE framework. It contains base classes
and an example. Framework is composed of works and Modules. User
implements Modules which manage one or more
(co-)processor(s). Framework receives a request from clients and
passes this information to Module through addWork() call. Module then
creates an work object which is usually wraps a set of algorithms to
be applied to given data. Work object then can be executed either
synchronously by module, or added to a queue which is consumed by a
different thread.

On client side, user creates a data object, which is composed of a
buffer containing the input data to be processed in Server and some
extra information such as which module to use and which
algorithm/operation to execute. When run Gaudi/Athena like
environments, most of the time user just needs to fill algorithm
field, other fields can be filled by a service inside the
framework. For standalone clients, framework requires a unique id
(token) for each outstanding request from the same client. There is
some more documentation in BufferContainer.hpp.

* Example client and module

dct8x8 example included in examples directory is a CUDA sample
distributed with CUDA SDK samples/3_Imaging/dct8x8, wrapped by ape
Module and Work objects as an example of integration into APE
framework. Original code is kept as it is except a couple of places to
have cuda and c++11 working together. 

Changes to original code are 
- moving #define's in Common.h to a separate file to drop cuda headers in c++11 code 
- Using new file in BmpUtils.cpp instead of Common.h since cuda headers are not needed there.
- Creating a new CMakeLists.txt file and moving original file to CMakeLists.txt.orig.

dct8x8Module object is an extremely simple module just constructs the
Work objects and runs them sychronously. For more advanced tasks, it
should query the details of the device and make better use of the
resource such as using multiple streams and arenas/buffers inside the
GPU as well as asynchronous job execution.

dctCudaWork1 and dctCudaWork2 work objects wrap cuda kernel calls
WrapperCUDA1 and WrapperCUDA2. These two cuda functions are copied from dct8x8.cu to
cudaFuncs.cu file and compiled as object files. Then Work objects link
to these cuda object files to combine c++11 and cuda.

dctClient manages reading the files, preparing the buffers to be
operated upon, setting token and algorithm, sending the file and
receiving the results back. It is an example of out-of-framework
algorithm development and testing. Under normal operation the work
done by the client is shared between offload service in experiments
framework and the algorithm requesting offload. 


Sami Kama

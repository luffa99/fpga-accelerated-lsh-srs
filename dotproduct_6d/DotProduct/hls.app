<project xmlns="com.autoesl.autopilot.project" top="vadd" name="DotProduct">
    <includePaths/>
    <libraryPaths/>
    <Simulation argv="">
        <SimFlow name="csim" ldflags="-L$(XILINX_XRT)/lib -lxilinxopencl -pthread -lrt" csimMode="2" lastCsimMode="2"/>
    </Simulation>
    <files xmlns="">
        <file name="../../host" sc="0" tb="1" cflags=" -Wno-unknown-pragmas" csimflags=" -Wno-unknown-pragmas" blackbox="false"/>
        <file name="xrt.ini" sc="0" tb="false" cflags="" csimflags="" blackbox="false"/>
        <file name="src/vadd.cpp" sc="0" tb="false" cflags="" csimflags="" blackbox="false"/>
        <file name="utils.mk" sc="0" tb="false" cflags="" csimflags="" blackbox="false"/>
        <file name="synthesis.tcl" sc="0" tb="false" cflags="" csimflags="" blackbox="false"/>
        <file name="src/host.hpp" sc="0" tb="false" cflags="" csimflags="" blackbox="false"/>
        <file name="src/host.cpp" sc="0" tb="false" cflags="" csimflags="" blackbox="false"/>
        <file name="Makefile" sc="0" tb="false" cflags="" csimflags="" blackbox="false"/>
    </files>
    <solutions xmlns="">
        <solution name="solution1" status="active"/>
    </solutions>
</project>


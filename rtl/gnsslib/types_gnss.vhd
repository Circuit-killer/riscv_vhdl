-----------------------------------------------------------------------------
--! @file
--! @copyright Copyright 2015 GNSS Sensor Ltd. All right reserved.
--! @author    Sergey Khabarov - sergeykhbr@gmail.com
--! @brief     Components declaration of the types_gnss package..
-----------------------------------------------------------------------------

--! Standard library
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

--! Math and data type transforamtion library
library commonlib;
use commonlib.types_common.all;

--! Leon3 technologies definition library
library techmap;
use techmap.gencomp.all;  -- technology enumerator

--! AMBA AXI4 interconnect types
library ambalib;
use ambalib.types_amba4.all;

--! @brief Declaration of the GNSS modules visible outside of the package.
--! @details This package provides User's API and general configuration 
--!          of the library. User should directly include this package into
--!          developing System with appropriate configuration values.
--!          Configuraiton constants allow to enable/disable GNSS systems
--!          separetedly, change channels configuration and flexibly tune
--!          Fast Search Engines.
--!
--! This library provides several key features:
--!   <ul>
--!     <li>Control external RF-boards including loading synthezises</li>
--!     <li>DSP of the input quantized samples such as correlation with 
--!         reference signals</li>
--!     <li>Generate the raw measurements independenetly from the firmware</li>
--!     <li>Implement hardware symbol synchronization</li>
--!     <li>Hardware timescale counter for different systems</li>
--!     <li>Implementing multi-path mitigation algrothims</li>
--!     <li>Noise estimators</li>
--!     <li>Multi-systems Fast Engines component</li>
--!   </ul>
package types_gnss is

  --! Maximal input bus width per complex component
  constant ADC_BIT_WIDTH             : integer := 2;

  --! GPS FSE module enable/disable
  constant CFG_GNSSLIB_FSEGPS_ENABLE : boolean := true;

  --! @brief Recording time interval for the FSE samples in msec.
  --! @details Fast Search Engine processing is splitted on two phases:
  --!          recording samples, post-processing. Duration of the recording
  --!          phase defines memory requirements and maximal accumulation
  --!          interval. Each milliseconds requires 2 KB of the dual-port RAM.
  constant CFG_FSE_MSEC_MAX          : integer := 16;

  --! Instantiated number of the GPS L1-CA channels. 0 is the valid value.
  constant CFG_GNSS_GPS_L1CA_NUM     : integer := 8;--12;
  --! Instantiated number of the Glonass L1-CA channels. 0 is the valid value.
  constant CFG_GNSS_GLONASS_L1_NUM   : integer := 8;--12;
  --! Instantiated number of the SBAS L1-CA channels. 0 is the valid value.
  constant CFG_GNSS_SBAS_L1_NUM      : integer := 0;--2; 
  --! Instantiated number of the Galileo E1 channels. 0 is the valid value.
  constant CFG_GNSS_GALILEO_E1_NUM   : integer := 0;--6;
  --! @brief Total number of the instantiated channels
  --! @details This value computes as a sum of all channels and is limited
  --!          only by address bus width that connects System Bus and GNSS
  --!          Top level. 
  --! @todo    refine maximal possible number
  constant CFG_GNSS_CHANNELS_TOTAL   : integer := (CFG_GNSS_GPS_L1CA_NUM
                                                + CFG_GNSS_GLONASS_L1_NUM 
                                                + CFG_GNSS_SBAS_L1_NUM 
                                                + CFG_GNSS_GALILEO_E1_NUM);
  --! Total number of GNSS timers. Shouldn't be modified by user.
  constant CFG_GNSS_TIMERS_TOTAL     : integer := 1;
  --! Total number of GNSS noise estimators. Shouldn't be modified by user.
  constant CFG_GNSS_NOISE            : integer := 1;
  --! Total number of Misc. modules. Shouldn't be modified by user.
  constant CFG_GNSS_MISC             : integer := 1;

  --! Total number of GNSS modules connected to internal Core Simplified Bus
  --! (SCB)
  constant CFG_GNSS_MODULES_TOTAL    : integer := CFG_GNSS_CHANNELS_TOTAL
                                                + CFG_GNSS_TIMERS_TOTAL
                                                + CFG_GNSS_NOISE
                                                + CFG_GNSS_MISC;
  constant CFG_GNSS_DWORD_PER_MODULE : integer := 8;
  constant CFG_GNSS_ADDR_WIDTH       : integer := log2(CFG_GNSS_MODULES_TOTAL)+6;

  constant MODULE_ID_MISC            : integer := 0;
  constant MODULE_ID_GLB_TIMER       : integer := MODULE_ID_MISC+CFG_GNSS_MISC;
  constant MODULE_ID_NOISE           : integer := MODULE_ID_GLB_TIMER+CFG_GNSS_TIMERS_TOTAL;
  constant MODULE_ID_CHN             : integer := MODULE_ID_NOISE + CFG_GNSS_NOISE;

  constant GEN_SYSTEM_GPSCA          : integer := 0;
  constant GEN_SYSTEM_GLOCA          : integer := GEN_SYSTEM_GPSCA+1;
  constant GEN_SYSTEM_SBAS           : integer := GEN_SYSTEM_GLOCA+1;
  constant GEN_SYSTEM_GALE1          : integer := GEN_SYSTEM_SBAS+1;
  constant GEN_SYSTEM_TOTAL          : integer := GEN_SYSTEM_GALE1+1;
  
  type gnss_system_type is array (0 to GEN_SYSTEM_TOTAL) of integer;
  constant fse_section_num : gnss_system_type := (GEN_SYSTEM_GLOCA => 1, others => 4);

  constant CFG_GNSS_PRNROM_BUSWIDTH  : integer := 5+8;--256 words(pilot+data) x 32 prn
  -- Galileo E1-pilot overlay code:
  constant CODE_CS25_1  : std_logic_vector(24 downto 0) := "0011100000001010110110010";
  constant CODE_CS25_1I : std_logic_vector(24 downto 0) := "1100011111110101001001101";

  
  type sys_parameter is array (0 to GEN_SYSTEM_TOTAL-1) of integer;

  ------------------------------------------------------------------------------
  -- GNSS Engine, top level
  component gnssengine is
  generic
  (
    tech   : integer range 0 to NTECH := 0;
    xaddr  : integer := 0;
    xmask  : integer := 16#FFFFF#;
    xirq   : integer := 0
  );
  port
  (
    nrst         : in std_logic;
    clk_bus      : in std_logic;
    clk_adc      : in std_logic;
    o_cfg        : out nasti_slave_config_type;
    i_axi        : in  nasti_slave_in_type;
    o_axi        : out nasti_slave_out_type;
    i_gps_I      : in std_logic_vector(1 downto 0);
    i_gps_Q      : in std_logic_vector(1 downto 0);
    i_glo_I      : in std_logic_vector(1 downto 0);
    i_glo_Q      : in std_logic_vector(1 downto 0);
    o_ms_pulse   : out std_logic;
    o_pps        : out std_logic
  );
  end component;


  ------------------------------------------------------------------------------
  -- Fast Search Engine v.2 (GPS only, 32 channels)
  component TopFSE is
  generic
  (
    tech   : integer := 0;
    xaddr  : integer := 0;
    xmask  : integer := 16#FFFFF#;
    sys    : integer := GEN_SYSTEM_GPSCA
  );
  port (
    nrst         : in std_logic;
    clk_bus      : in std_logic;
    clk_adc      : in std_logic;
    o_cfg        : out nasti_slave_config_type;
    i_axi        : in  nasti_slave_in_type;
    o_axi        : out nasti_slave_out_type;
    i_I          : in std_logic_vector(1 downto 0);
    i_Q          : in std_logic_vector(1 downto 0);
    i_ms_pulse   : in std_logic;
    i_pps        : in std_logic;
    i_2ms_only   : in std_logic
  );
  end component;

  --! @brief     RF-front controller based on MAX2769 ICs.
  --! @details   This unit implements SPI interface with MAX2769 ICs
  --!            and interacts with the antenna control signals.
  component axi_rfctrl is
  generic (
    xaddr    : integer := 0;
    xmask    : integer := 16#fffff#
  );
  port (
    nrst    : in  std_logic;
    clk    : in  std_logic;
    o_cfg  : out nasti_slave_config_type;
    i_axi   : in  nasti_slave_in_type;
    o_axi   : out nasti_slave_out_type;
    i_gps_ld : in std_logic;
    i_glo_ld : in std_logic;
    --! @name  Synthezator's SPI interface signals:
    --! @brief Connects to MAX2769 IC.
    --! @{
    outSCLK  : out std_logic;
    outSDATA : out std_logic;
    outCSn   : out std_logic_vector(1 downto 0);
    --! @}
    
    --! @name  Antenna control signals:
    --! @brief RF front-end IO analog signals.
    --! @{
    inExtAntStat   : in std_logic;
    inExtAntDetect : in std_logic;
    outExtAntEna   : out std_logic;
    outIntAntContr   : out std_logic
    --! @}
  );
end component; 

  component axi_recorder is generic (
    tech     : integer := 0;
    xaddr    : integer := 0;
    xmask    : integer := 16#ffff0#
  );
  port (
    nrst    : in  std_logic;
    clk_bus : in  std_logic;
    clk_adc : in  std_logic;
    o_cfg   : out nasti_slave_config_type;
    i_axi   : in  nasti_slave_in_type;
    o_axi   : out nasti_slave_out_type;
    i_gps_I : in std_logic_vector(1 downto 0);
    i_gps_Q : in std_logic_vector(1 downto 0)
  );
  end component; 


  ------------------------------------------------------------------------------
  -- 3-axis STMicroelectronics Gyroscope SPI controller (4-wires mode)

  component gyrospi is
  generic (
    xaddr    : integer := 0;
    xmask    : integer := 16#fffff#
  );
  port (
    rst    : in  std_ulogic;
    clk    : in  std_ulogic;
    i_axi  : in  nasti_slave_in_type;
    o_axi  : out nasti_slave_out_type;
    inInt1 : in std_ulogic;
    inInt2 : in std_ulogic;
    inSDI  : in std_ulogic;
    outSPC : out std_ulogic;
    outSDO : out std_ulogic;
    outCSn : out std_ulogic
  );
  end component; 

  ------------------------------------------------------------------------------
  -- 3-axis STMicroelectronics Accelerometer SPI controller (4-wires mode)

  component accelspi is
    generic (
      xaddr    : integer := 0;
      xmask    : integer := 16#fffff#
    );
    port (
      rst    : in  std_ulogic;
      clk    : in  std_ulogic;
      i_axi  : in  nasti_slave_in_type;
      o_axi  : out nasti_slave_out_type;
      inInt1 : in std_ulogic;
      inInt2 : in std_ulogic;
      inSDI  : in std_ulogic;
      outSPC : out std_ulogic;
      outSDO : out std_ulogic;
      outCSn : out std_ulogic
    );
  end component; 


 

end;

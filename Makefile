##### click-path ####
CLICK_PATH =/usr/local/src/fastclick-master
USERLEVEL_PATH = $(CLICK_PATH)/userlevel
FASTUSERLEVEL_PATH = /usr/local/src/fastclick-master/userlevel
DPDK_PATH=/usr/local/src/dpdk-stable-16.11.6
DPDK_PATH_INCLUDES=$(DPDK_PATH)/x86_64-native-linuxapp-gcc/include
CLICK_INCLUDES = -I$(CLICK_PATH)/include -I$(CLICK_PATH) -I$(DPDK_PATH)/lib -I$(DPDK_PATH_INCLUDES)

#### SGX SDK settings ####
SGX_SDK ?= /opt/intel/sgxsdk
SGX_MODE ?= SIM
SGX_ARCH ?= x64

ifeq ($(shell getconf LONG_BIT), 32)
	SGX_ARCH := x86
else ifeq ($(findstring -m32, $(CXXFLAGS)), -m32)
	SGX_ARCH := x86
endif

ifeq ($(SGX_ARCH), x86)
	SGX_COMMON_CFLAGS := -m32
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib
	SGX_ENCLAVE_SINGER := $(SGX_SDK)/bin/x86/sgx_sign
	SGX_EDGERR8R := $(SGX_SDK)/bin/x86/sgx_edger8r
else
	SGX_COMMON_CFLAGS := -m64
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib64
	SGX_ENCLAVE_SINGER := $(SGX_SDK)/bin/x64/sgx_sign
	SGX_EDGERR8R := $(SGX_SDK)/bin/x64/sgx_edger8r
endif

ifeq ($(SGX_DEBUG), 1)
	SGX_COMMON_CFLAGS += -O0 -g
else
	SGX_COMMON_CFLAGS += -O2
endif


#### App settings ####

ifneq ($(SGX_MODE), HW)
	Urts_Library_Name := sgx_urts_sim
else
	Urts_Library_Name := sgx_urts
endif

App_Cpp_Files := app/manager.cc 
App_Include_Path := -Iapp -I$(SGX_SDK)/include $(CLICK_INCLUDES) -Ienclave/lib

App_C_Flags := $(SGX_COMMON_CFLAGS) -fPIC -Wno-attributes $(App_Include_Path) 

# Three configuration modes - Debug, prerelease, release
#   Debug - Macro DEBUG enabled.
#   Prerelease - Macro NDEBUG and EDEBUG enabled.
#   Release - Macro NDEBUG enabled.
ifeq ($(SGX_DEBUG), 1)
	App_C_Flags += -DDEBUG -UNDEBUG -UEDEBUG
else ifeq ($(SGX_PRERELEASE), 1)
	App_C_Flags += -DNDEBUG -DEDEBUG -UDEBUG
else
	App_C_Flags += -DNDEBUG -UEDEBUG -UDEBUG
endif


App_Cpp_Flags := $(App_C_Flags) -std=c++11
App_Link_Flags := $(SGX_COMMON_CFLAGS) -L$(SGX_LIBRARY_PATH) -l$(Urts_Library_Name) -lpthread

ifneq ($(SGX_MODE), HW)
	App_Link_Flags += -lsgx_uae_service_sim
else
	App_Link_Flags += -lsgx_uae_service
endif

App_Cpp_Objects :=$(App_Cpp_Files:.cc=.o)

App_Name := app




#### Enclave settings ####

ifneq ($(SGX_MODE), HW)
	Trts_Library_Name := sgx_trts_sim
	Service_Library_Name := sgx_tservice_sim
	Crypto_Library_Name := sgx_tcrypto
else
	Trts_Library_Name := sgx_trts
	Service_Library_Name := sgx_tservice
	Crypto_Library_Name := sgx_tcrypto
endif

Enclave_Cpp_Files := $(wildcard enclave/*.c) $(wildcard enclave/lib/*.c) $(wildcard enclave/element/*.c)
Enclave_Include_Paths := -Ienclave -Ienclave/net  -Ienclave/lib  -I$(SGX_SDK)/include -I$(SGX_SDK)/include/tlibc -I$(SGX_SDK)/include/stlport


Enclave_C_Flags := $(SGX_COMMON_CFLAGS) -nostdinc -fvisibility=hidden -fpie -fstack-protector $(Enclave_Include_Paths)
Enclave_Cpp_Flags := $(Enclave_C_Flags) -std=c++03 -nostdinc++

Enclave_Link_Flags := $(SGX_COMMON_CFLAGS) -Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles -L$(SGX_LIBRARY_PATH) \
	-Wl,--whole-archive -l$(Trts_Library_Name) -Wl,--no-whole-archive \
	-Wl,--start-group -lsgx_tstdc -lsgx_tstdcxx -l$(Crypto_Library_Name) -l$(Service_Library_Name) -Wl,--end-group \
	-Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
	-Wl,-pie,-eenclave_entry -Wl,--export-dynamic  \
	-Wl,--defsym,__ImageBase=0 \
	-Wl,--version-script=enclave/enclave.lds

Enclave_Cpp_Objects := $(Enclave_Cpp_Files:.c=.o)

Enclave_name := enclave.so
Signed_Enclave_Name := enclave.signed.so
Enclave_Config_File := enclave/enclave.config.xml

ifeq ($(SGX_MODE), HW)
ifneq ($(SGX_DEBUG), 1)
ifneq ($(SGX_PRERELEASE), 1)
	Build_Mode = HW_RELEASE
endif
endif
endif

.PHONY: all run

ifeq ($(Build_Mode), HW_RELEASE)
all: $(App_Name) $(Enclave_name)
	@echo "The project has been built in release hardware mode."
	@echo "Please sign the $(Enclave_Name) first with your signing key before you run the $(App_Name) to launch and access the enclave."
	@echo "To sign the enclave use the command:"
	@echo "   $(SGX_ENCLAVE_SIGNER) sign -key <your key> -enclave $(Enclave_Name) -out <$(Signed_Enclave_Name)> -config $(Enclave_Config_File)"
	@echo "You can also sign the enclave using an external signing tool. See User's Guide for more details."
	@echo "To build the project in simulation mode set SGX_MODE=SIM. To build the project in prerelease mode set SGX_PRERELEASE=1 and SGX_MODE=HW."
else
all: $(App_Name) $(Signed_Enclave_Name)
endif

run: all
ifneq ($(Build_Mode), HW_RELEASE)
	@$(CURDIR)/$(App_Name)
	@echo "RUN  =>  $(App_Name) [$(SGX_MODE)|$(SGX_ARCH), OK]"
endif



#### App Objects ####
app/enclave_u.c: $(SGX_EDGERR8R) enclave/enclave.edl
	@cd app && $(SGX_EDGERR8R) --untrusted enclave.edl --search-path ../enclave --search-path $(SGX_SDK)/include 
	@echo "GEN => $@"

app/enclave_u.o: app/enclave_u.c
	@$(CC) $(App_C_Flags) -c $< -o $@
	@echo "CC <= @<"
	
app/%.o: app/%.cc
	@$(CXX) $(App_Cpp_Flags) -c $< -o $@
	@echo "CXX <= @<"

$(App_Name): app/enclave_u.o $(App_Cpp_Objects)
	#@$(CXX) $^ -o $@ $(App_Link_Flags)
	#@echo "LINK => $@"
	@echo "We link the manager element with click later"

#### Enclave Objects ####
enclave/enclave_t.c: $(SGX_EDGERR8R) enclave/enclave.edl
	@cd enclave && $(SGX_EDGERR8R) --trusted enclave.edl --search-path ../app --search-path $(SGX_SDK)/include 
	@echo "GEN => $@"
enclave/enclave_t.o: enclave/enclave_t.c
	@$(CC) $(Enclave_C_Flags) -c $< -o $@
	@echo "GEN => $@"
enclave/%.o: enclave/%.c
	@$(CXX) $(Enclave_Cpp_Flags) -c $< -o $@
	@echo "GEN => $@"

$(Enclave_name): enclave/enclave_t.o $(Enclave_Cpp_Objects)
	$(CXX) $^ -o $@ $(Enclave_Link_Flags)
	@echo "LINK => $@"

$(Signed_Enclave_Name): $(Enclave_name)
	@$(SGX_ENCLAVE_SINGER) sign -key enclave/enclave_private.pem -enclave $(Enclave_name) \
	-out $@ -config $(Enclave_Config_File)
	@echo "SIGN =>$@"

.PHONY: clean install
install:
	@cp app/*.o enclave/enclave_private.pem *.so Makefile-SGX $(FASTUSERLEVEL_PATH)
	@echo "cp manager element object and enclave and privatekey to $(USERLEVEL_PATH)/"
	@cd $(FASTUSERLEVEL_PATH) && $(MAKE) -f Makefile-SGX
clean:
	@rm -f  $(Enclave_name) $(Signed_Enclave_Name) $(App_Cpp_Objects) $(Enclave_Cpp_Objects) \
	app/enclave_u.* enclave/enclave_t.* 
	
	

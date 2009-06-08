#----------------------------------------------------------------------------
#
#        db-loader module makefile
#
#----------------------------------------------------------------------------

M_INCL_DIR = ../include
M_LIB_DIR  = ../lib
M_BIN_DIR  = ../bin

PROJ_DIR   = .

L_INCL_DIR = $(PROJ_DIR)/include
SRC_DIR    = $(PROJ_DIR)/src
OBJ_DIR    = $(PROJ_DIR)/obj
L_LIB_DIR  = $(PROJ_DIR)/lib
L_BIN_DIR  = $(PROJ_DIR)/bin
TEST_DIR   = $(PROJ_DIR)/test

VPATH = $(OBJ_DIR) $(L_BIN_DIR)

EXT_INCLS_DIRS_OPT =
EXT_LIBS_DIRS_OPT =
EXT_LIBS_OPT      =

#----------------------------------------------------------------------------
# LINCLUDES and LDEFINES are appended to CFLAGS and C++FLAGS
#----------------------------------------------------------------------------
LDEFINES  =
LINCLUDES = -I$(L_INCL_DIR) -I$(M_INCL_DIR) $(EXT_INCLS_DIRS_OPT)

#----------------------------------------------------------------------------
# Include the appropriate compiler/platform definitions ...
#----------------------------------------------------------------------------
include ../etc/Makefile.platform

# Dependent libraries for the db-loader module
CIF_FILE_UTIL_LIB = $(M_LIB_DIR)/cif-file-util.a
CIF_FILE_LIB      = $(M_LIB_DIR)/cif-file.a
CIFPARSE_LIB      = $(M_LIB_DIR)/cifparse-obj.a
TABLES_LIB        = $(M_LIB_DIR)/tables.a
COMMON_LIB        = $(M_LIB_DIR)/common.a
REGEX_LIB         = $(M_LIB_DIR)/regex.a

ALL_DEP_LIBS = $(CIF_FILE_UTIL_LIB) $(CIF_FILE_LIB) $(CIFPARSE_LIB) \
  $(TABLES_LIB) $(COMMON_LIB) $(REGEX_LIB)

# Module libraries
MOD_LIB = schema-map.a

# Aggregate library
AGR_LIB = all.a

# Temporary library. Used to obtain the agregate library.
TMP_LIB = tmp.a

L_MOD_LIB = $(L_LIB_DIR)/$(MOD_LIB)
M_MOD_LIB = $(M_LIB_DIR)/$(MOD_LIB)
L_AGR_LIB = $(L_LIB_DIR)/$(AGR_LIB)
M_AGR_LIB = $(M_LIB_DIR)/$(AGR_LIB)

# Base main file names. Must have ".ext" at the end of the file.
BASE_MAIN_FILES = 

# Base other file names. Must have ".ext" at the end of the file.
BASE_OTHER_FILES = SchemaMap.ext \
                   SchemaDataInfo.ext \
                   SchemaParentChild.ext


# Base header files. Replace ".ext" with ".h"
HEADER_BASE_OTHER_FILES = ${BASE_OTHER_FILES:.ext=.h}

# Other extra header files
HEADER_EXTRA_FILES =

# All header files
HEADER_FILES = $(HEADER_BASE_OTHER_FILES) $(HEADER_EXTRA_FILES)


# Main source files. Replace ".ext" with ".C"
SRC_MAIN_FILES = ${BASE_MAIN_FILES:.ext=.C}

# Other source files from base. Replace ".ext" with ".C"
SRC_BASE_OTHER_FILES = ${BASE_OTHER_FILES:.ext=.C}

# Other extra source files
SRC_EXTRA_FILES = SchemaMapCreate.C

# Non-main source files
SRC_OTHER_FILES = $(SRC_BASE_OTHER_FILES) $(SRC_EXTRA_FILES)

# All source files
SRC_FILES = $(SRC_MAIN_FILES) $(SRC_OTHER_FILES)


# Main object files. Replace ".ext" with ".o"
OBJ_MAIN_FILES = ${BASE_MAIN_FILES:.ext=.o}

# Other object files. Replace ".C" with ".o"
OBJ_OTHER_FILES = ${SRC_OTHER_FILES:.C=.o}

# All module's object files
OBJ_FILES = $(OBJ_MAIN_FILES) $(OBJ_OTHER_FILES)

# All object files in the current directory
ALL_OBJ_FILES = *.o


# Executables. Remove ".ext"
TARGETS = ${BASE_MAIN_FILES:.ext=}

# Scripts
TARGET_SCRIPTS =

# Test related files
TEST_FILES = 

.PHONY: ../etc/Makefile.platform all install test export clean clean_build clean_test

.PRECIOUS: $(OBJ_DIR)/%.o


# All
all: install


# Installation
install: $(M_MOD_LIB)


# Test
test: all


export:
#	Specify your own export statements
	mkdir -p $(EXPORT_DIR)
	@cp Makefile $(EXPORT_DIR)
	@cd $(EXPORT_DIR); mkdir -p $(L_INCL_DIR)
	@cd $(L_INCL_DIR); $(EXPORT) $(EXPORT_LIST) $(HEADER_FILES) ../$(EXPORT_DIR)/$(L_INCL_DIR)
	@cd $(EXPORT_DIR); mkdir -p $(SRC_DIR)
	@cd $(SRC_DIR); $(EXPORT) $(EXPORT_LIST) $(SRC_FILES) ../$(EXPORT_DIR)/$(SRC_DIR)
	@cd $(EXPORT_DIR); mkdir -p $(OBJ_DIR)
	@cd $(EXPORT_DIR); mkdir -p $(L_LIB_DIR)


clean: clean_build clean_test


# Special rules for making executables


# General rule for making executables
%: $(OBJ_DIR)/%.o $(M_MOD_LIB) $(ALL_DEP_LIBS)
	$(CCC) $(EXT_LIBS_DIRS_OPT) $(LDFLAGS) $< $(M_MOD_LIB) $(ALL_DEP_LIBS) $(EXT_LIBS_OPT) $(MALLOCLIB) -lm -o $(L_BIN_DIR)/$@
	@cp -f $(L_BIN_DIR)/$@ $(M_BIN_DIR)/$@


# Rule for build cleaning
clean_build:
	@cd $(M_INCL_DIR); rm -f $(HEADER_FILES)
	@rm -f $(OBJ_DIR)/*.o
	@rm -rf $(OBJ_DIR)/ii_files
	@rm -f $(L_MOD_LIB)
	@rm -f $(M_MOD_LIB)
	@rm -f $(M_AGR_LIB)


# Rule for test results cleaning
clean_test:

# Rule for making module library in master library directory
$(M_MOD_LIB): $(L_MOD_LIB)
#       Install header files
	@cd $(L_INCL_DIR); \
          ../$(INSTALL) $(INSTALLOPTS) $(HEADER_FILES) ../$(M_INCL_DIR)

#       Install module library
	$(INSTALL) $(INSTALLOPTS) $(L_MOD_LIB) $(M_LIB_DIR)

#       Create agregate library

	@cd $(M_LIB_DIR); ../etc/initlib.sh $(MOD_LIB)

	@cd $(L_LIB_DIR); cp ../$(M_AGR_LIB) $(TMP_LIB)
	@cd $(L_LIB_DIR); $(AR) $(AR_GETFLAGS) $(TMP_LIB)
	@cd $(L_LIB_DIR); rm -f $(TMP_LIB)

	@cd $(L_LIB_DIR); cp $(MOD_LIB) $(TMP_LIB)
	@cd $(L_LIB_DIR); $(AR) $(AR_GETFLAGS) $(TMP_LIB)
	@cd $(L_LIB_DIR); rm -f $(TMP_LIB)

	@cd $(L_LIB_DIR); $(AR) $(AR_PUTFLAGS) $(AGR_LIB) $(ALL_OBJ_FILES)
	@cd $(L_LIB_DIR); rm -f $(ALL_OBJ_FILES)

	$(INSTALL) $(INSTALLOPTS) $(L_AGR_LIB) $(M_LIB_DIR)
	@rm -f $(L_AGR_LIB)


# Rule for making module library in local library directory
$(L_MOD_LIB): $(OBJ_OTHER_FILES)
#       Create module library
	@cd $(OBJ_DIR); $(AR) $(AR_PUTFLAGS) ../$@ $(OBJ_OTHER_FILES)
	$(RANLIB) $@
	@echo $@ " is up to date."


# Special rules for making object files


# General rule for making object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.C
	$(CCC) $(C++FLAGS) -c $< -o $@


# Phony rule for making object files
%.o: $(SRC_DIR)/%.C
	$(CCC) $(C++FLAGS) -c $< -o $(OBJ_DIR)/$@


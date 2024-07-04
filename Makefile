MODULE_big = logical_ddl
OBJS = hook_ddl.o ddl_commands.o ddl_parser.o
EXTENSION = logical_ddl
DATA = logical_ddl--1.0.sql
PGFILEDESC = "logical replication of ddl command"
REGRESS = logical_ddl

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/logical_ddl
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

EXTENSION = logical_ddl

MODULES = ddl_deparce

PG_CONFIG = pg_config

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
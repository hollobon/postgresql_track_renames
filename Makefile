MODULES = track_renames

EXTENSION = track_renames
DATA = track_renames--0.1.sql
PGFILEDESC = "track renames extension"

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

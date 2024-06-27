#include "postgres.h"
#include "executor/spi.h"
#include "publication.h"

void create_publication(const char *table_name) {
    int ret;
    StringInfoData query;

    SPI_connect();

    initStringInfo(&query);
    appendStringInfo(&query, "CREATE PUBLICATION logical_ddl_publication FOR TABLE %s;", table_name);

    ret = SPI_execute(query.data, false, 0);
    if (ret != SPI_OK_UTILITY) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("Failed to create publication for logical replication")));
    }

    SPI_finish();
}

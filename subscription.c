#include "postgres.h"
#include "executor/spi.h"
#include "subscription.h"

void create_subscription(const char *conninfo) {
    StringInfoData query;
    int ret;

    SPI_connect();
    
    initStringInfo(&query);
    appendStringInfo(&query, "CREATE SUBSCRIPTION logical_ddl_subscription CONNECTION '%s' PUBLICATION logical_ddl_publication;", conninfo);
    
    ret = SPI_execute(query.data, false, 0);
    if (ret != SPI_OK_UTILITY) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("Failed to create subscription for logical replication")));
    }
    SPI_finish();
}

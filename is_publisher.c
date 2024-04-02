#include "postgres.h"

#include "fmgr.h"
#include "array.h"
#include "pg_list.h"
#include "replication/logical.h"
#include "syscache.h"
#include "elog.h"
#include "catalog/pg_subscription.h"
#include "miscadmin.h"
#include "utils/builtins.h"
#include "utils/builtins.h"
#include "utils/rel.h"
#include "utils/relcache.h"
#include "access/genam.h"
#include "pg_subscription_rel_d.h"
#include "catalog/pg_subscription_rel.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(is_subscription_subscriber);

Datum
is_owner(PG_FUNCTION_ARGS) {
    Oid subid = PG_GETARG_OID(0);
    Oid current_db = MyDatabaseId;
//    bool is_subscriber = false;
    bool is_owner = false;
    Subscription *sub;

    sub = GetSubscription(subid, false);
    if (!sub)
        ereport(ERROR,
                (errcode(ERRCODE_UNDEFINED_OBJECT),
                 errmsg("subscription with OID %u does not exist", subid)));

    if (sub->owner == GetUserId())
    {
        is_owner = true;

        // Relation pg_subscription_rel = table_open(SubscriptionRelRelationId, AccessShareLock);

        // SysScanDesc scan = systable_beginscan(pg_subscription_rel, InvalidOid, false, NULL, 0, NULL);

        // HeapTuple tuple;
        // while (HeapTupleIsValid(tuple = systable_getnext(scan)))
        // {
        //     Form_pg_subscription_rel sub_rel = (Form_pg_subscription_rel)GETSTRUCT(tuple);

        //     // Проверяем, совпадает ли OID подписки и OID текущей базы данных
        //     if (sub_rel->srsubid == subid && sub_rel->srsubdbid == current_db)
        //     {
        //         is_subscriber = true;
        //         break;
        //     }
        // }


        // systable_endscan(scan);
        // table_close(pg_subscription_rel, AccessShareLock);
    }

    FreeSubscription(sub);

    // if (!is_owner)
    // {
    //     ereport(ERROR,
    //             (errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
    //              errmsg("current user is not the owner of subscription with OID %u", subid)));
    // }
    // else if (!is_subscriber)
    // {
    //     ereport(ERROR,
    //             (errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
    //              errmsg("current database is not a subscriber of subscription with OID %u", subid)));
    // }

    PG_RETURN_BOOL(is_owner);
}

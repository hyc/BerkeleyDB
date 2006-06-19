/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2006
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: mp_mvcc.c,v 12.8 2006/06/19 14:55:37 mjc Exp $
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"
#include "dbinc/txn.h"

static int __pgno_cmp __P((const void *, const void *));

/*
 * __memp_bh_priority --
 *	Get the the aggregate priority of a chain of buffer headers.
 *
 * PUBLIC: u_int32_t __memp_bh_priority __P((BH *));
 */
u_int32_t
__memp_bh_priority(bhp)
	BH *bhp;
{
	u_int32_t priority;

	if (bhp == NULL)
		return (0);

	while (SH_CHAIN_NEXT(bhp, vc, __bh) != NULL)
		bhp = SH_CHAIN_NEXT(bhp, vc, __bh);

	priority = bhp->priority;

	while ((bhp = SH_CHAIN_PREV(bhp, vc, __bh)) != NULL)
		if (bhp->priority < priority)
			priority = bhp->priority;

	return (priority);
}

/*
 * __memp_bucket_reorder --
 *	Adjust a hash queue so that the given bhp is in priority order.
 *
 * PUBLIC: void __memp_bucket_reorder __P((DB_MPOOL_HASH *, BH *));
 */
void
__memp_bucket_reorder(hp, bhp)
	DB_MPOOL_HASH *hp;
	BH *bhp;
{
	BH *fbhp, *prev;
	u_int32_t priority;

	/*
	 * Buffers on hash buckets are sorted by priority -- move the buffer to
	 * the correct position in the list.  We only need to do any work if
	 * there are two or more buffers in the bucket.
	 */
	if (bhp == NULL || (fbhp = SH_TAILQ_FIRST(&hp->hash_bucket, __bh)) ==
	     SH_TAILQ_LAST(&hp->hash_bucket, hq, __bh))
		goto done;

	while (SH_CHAIN_NEXT(bhp, vc, __bh) != NULL)
		bhp = SH_CHAIN_NEXT(bhp, vc, __bh);

	priority = __memp_bh_priority(bhp);

	if (fbhp == bhp)
		fbhp = SH_TAILQ_NEXT(fbhp, hq, __bh);
	SH_TAILQ_REMOVE(&hp->hash_bucket, bhp, hq, __bh);

	for (prev = NULL; fbhp != NULL;
	    prev = fbhp, fbhp = SH_TAILQ_NEXT(fbhp, hq, __bh))
		if (__memp_bh_priority(fbhp) > priority)
			break;
	if (prev == NULL)
		SH_TAILQ_INSERT_HEAD(&hp->hash_bucket, bhp, hq, __bh);
	else
		SH_TAILQ_INSERT_AFTER(&hp->hash_bucket, prev, bhp, hq, __bh);

done:	/* Reset the hash bucket's priority. */
	hp->hash_priority =
	    __memp_bh_priority(SH_TAILQ_FIRST(&hp->hash_bucket, __bh));
}

/*
 * __memp_bh_settxn --
 *	Set the transaction that owns the given buffer.
 *
 * PUBLIC: int __memp_bh_settxn __P((DB_MPOOL *, MPOOLFILE *mfp, BH *, void *));
 */
int __memp_bh_settxn(dbmp, mfp, bhp, vtd)
	DB_MPOOL *dbmp;
	MPOOLFILE *mfp;
	BH *bhp;
	void *vtd;
{
	DB_ENV *dbenv;
	TXN_DETAIL *td;

	dbenv = dbmp->dbenv;
	td = (TXN_DETAIL *)vtd;

	if (td == NULL) {
		__db_errx(dbenv,
		      "%s: non-transactional update to a multiversion file",
		    __memp_fns(dbmp, mfp));
		return (EINVAL);
	}

	if (bhp->td_off != INVALID_ROFF) {
		DB_ASSERT(dbenv, BH_OWNER(dbenv, bhp) == td);
		return (0);
	}

	bhp->td_off = R_OFFSET(&dbenv->tx_handle->reginfo, td);
	return (__txn_add_buffer(dbenv, td));
}

/*
 * __memp_skip_curadj --
 *	Indicate whether a cursor adjustment can be skipped for a snapshot
 *	cursor.
 *
 * PUBLIC: int __memp_skip_curadj __P((DBC *, db_pgno_t));
 */
int
__memp_skip_curadj(dbc, pgno)
	DBC * dbc;
	db_pgno_t pgno;
{
	BH *bhp;
	DB_ENV *dbenv;
	DB_MPOOL *dbmp;
	DB_MPOOL_HASH *hp;
	DB_MPOOLFILE *dbmfp;
	DB_TXN *txn;
	MPOOL *c_mp, *mp;
	MPOOLFILE *mfp;
	REGINFO *infop;
	roff_t mf_offset;
	u_int32_t n_cache;
	int skip;

	dbenv = dbc->dbp->dbenv;
	dbmp = dbenv->mp_handle;
	mp = dbmp->reginfo[0].primary;
	dbmfp = dbc->dbp->mpf;
	mfp = dbmfp->mfp;
	mf_offset = R_OFFSET(dbmp->reginfo, mfp);
	skip = 0;

	for (txn = dbc->txn; txn->parent != NULL; txn = txn->parent)
		;

	/*
	 * Determine the cache and hash bucket where this page lives and get
	 * local pointers to them.  Reset on each pass through this code, the
	 * page number can change.
	 */
	n_cache = NCACHE(mp, mf_offset, pgno);
	infop = &dbmp->reginfo[n_cache];
	c_mp = infop->primary;
	hp = R_ADDR(infop, c_mp->htab);
	hp = &hp[NBUCKET(c_mp, mf_offset, pgno)];

	MUTEX_LOCK(dbenv, hp->mtx_hash);
	SH_TAILQ_FOREACH(bhp, &hp->hash_bucket, hq, __bh) {
		if (bhp->pgno != pgno || bhp->mf_offset != mf_offset)
			continue;

		/*
		 * Snapshot reads -- get the version of the page that was
		 * visible *at* the read_lsn.  In particular, pages with commit
		 * LSN equal to the read LSN were not visible, so we need to
		 * skip those versions.
		 */
		if (!BH_OWNED_BY(dbenv, bhp, txn))
			skip = 1;
		break;
	}
	MUTEX_UNLOCK(dbenv, hp->mtx_hash);

	return (skip);
}

#define	DB_FREEZER_MAGIC 0x06102002

/*
 * __memp_bh_freeze --
 *	Save a buffer header to temporary storage in case it is needed later by
 *	a snapshot transaction.  This function should be called with the hash
 *	bucket locked, as it inserts a frozen buffer after writing the data.
 *
 * PUBLIC: int __memp_bh_freeze __P((DB_MPOOL *, REGINFO *, DB_MPOOL_HASH *,
 * PUBLIC:     BH *, int *));
 */
int
__memp_bh_freeze(dbmp, infop, hp, bhp, need_frozenp)
	DB_MPOOL *dbmp;
	REGINFO *infop;
	DB_MPOOL_HASH *hp;
	BH *bhp;
	int *need_frozenp;
{
	BH *frozen_bhp;
	BH_FROZEN_ALLOC *frozen_alloc;
	DB_ENV *dbenv;
	DB_FH *fhp;
	MPOOL *c_mp;
	MPOOLFILE *bh_mfp;
	db_pgno_t newpgno, nextfree;
	int nio, ret, t_ret;
	u_int32_t bytes, magic, mbytes, nbucket, ncache, pagesize;
	char filename[100], *real_name;

	dbenv = dbmp->dbenv;
	c_mp = infop->primary;
	*need_frozenp = ret = 0;
	/* Find the associated MPOOLFILE. */
	bh_mfp = R_ADDR(dbmp->reginfo, bhp->mf_offset);
	pagesize = bh_mfp->stat.st_pagesize;
	real_name = NULL;
	fhp = NULL;

	DB_ASSERT(dbenv, bhp->ref == 0);
	DB_ASSERT(dbenv, !F_ISSET(bhp, BH_FROZEN));

	++bhp->ref;
	F_SET(bhp, BH_LOCKED);
	MVCC_MPROTECT(bhp->buf, pagesize, PROT_READ | PROT_WRITE);

	MUTEX_UNLOCK(dbenv, hp->mtx_hash);
	MPOOL_REGION_LOCK(dbenv, infop);
	frozen_bhp = SH_TAILQ_FIRST(&c_mp->free_frozen, __bh);
	if (frozen_bhp != NULL)
		SH_TAILQ_REMOVE(&c_mp->free_frozen, frozen_bhp, hq, __bh);
	else {
		*need_frozenp = 1;

		/* There might be a small amount of unallocated space. */
		if (__db_shalloc(infop,
		    sizeof (BH_FROZEN_ALLOC) + sizeof (BH_FROZEN_PAGE), 0,
		    &frozen_alloc) == 0) {
			frozen_bhp = (BH *)(frozen_alloc + 1);
			SH_TAILQ_INSERT_HEAD(&c_mp->alloc_frozen, frozen_alloc,
			    links, __bh_frozen_a);
		}
	}
	MPOOL_REGION_UNLOCK(dbenv, infop);
	MUTEX_LOCK(dbenv, hp->mtx_hash);

	/*
	 * If we can't get an frozen buffer header, return ENOMEM immediately:
	 * we don't want to call __memp_alloc recursively.  __memp_alloc will
	 * turn the next free page it finds into frozen buffer headers.
	 */
	if (frozen_bhp == NULL) {
		ret = ENOMEM;
		goto err;
	}

	/*
	 * For now, keep things simple and have one file per page size per
	 * cache.  Concurrency will be suboptimal, but debugging should be
	 * simpler.
	 */
	ncache = (u_int32_t)(infop - dbmp->reginfo);
	nbucket = (u_int32_t)(hp - (DB_MPOOL_HASH *)R_ADDR(infop, c_mp->htab));
	snprintf(filename, sizeof(filename), "__db.freezer.%u.%u.%uK",
	    ncache, nbucket, pagesize / 1024);

	if ((ret = __db_appname(dbenv, DB_APP_NONE, filename,
	    0, NULL, &real_name)) != 0)
		goto err;
	if ((ret = __os_open_extend(dbenv, real_name, pagesize,
	    DB_OSO_CREATE | DB_OSO_EXCL, dbenv->db_mode, &fhp)) == 0) {
		/* We're creating the file -- initialize the metadata page. */
		magic = DB_FREEZER_MAGIC;
		newpgno = 0;
		if ((ret = __os_write(dbenv, fhp, &magic, sizeof(u_int32_t),
		    &nio)) < 0 || nio == 0 ||
		    (ret = __os_write(dbenv, fhp, &newpgno, sizeof(db_pgno_t),
		    &nio)) < 0 || nio == 0 ||
		    (ret = __os_seek(dbenv, fhp, 0, pagesize, 0, 0,
		    DB_OS_SEEK_SET)) != 0)
			goto err;
	} else if (ret == EEXIST)
		ret = __os_open_extend(dbenv, real_name, pagesize, 0,
		    dbenv->db_mode, &fhp);
	if (ret != 0)
		goto err;
	if ((ret = __os_read(dbenv, fhp, &magic, sizeof(u_int32_t),
	    &nio)) < 0 || nio == 0 ||
	    (ret = __os_read(dbenv, fhp, &newpgno, sizeof(db_pgno_t),
	    &nio)) < 0 || nio == 0)
		goto err;
	if (magic != DB_FREEZER_MAGIC) {
		ret = EINVAL;
		goto err;
	}
	if (newpgno == 0) {
		if ((ret = __os_ioinfo(dbenv, real_name, fhp, &mbytes, &bytes,
		    NULL)) != 0)
			goto err;
		newpgno = (db_pgno_t)(mbytes * (MEGABYTE / pagesize) +
		    ((bytes + pagesize - 1) / pagesize));
		if ((ret = __os_write(dbenv, fhp, &newpgno, sizeof(db_pgno_t),
		    &nio)) < 0 || nio == 0)
			goto err;
	} else {
		if ((ret = __os_seek(dbenv, fhp, newpgno, pagesize, 0, 0,
		    DB_OS_SEEK_SET)) != 0 ||
		    (ret = __os_read(dbenv, fhp, &nextfree, sizeof(db_pgno_t),
		    &nio)) < 0 || nio == 0)
			goto err;
		if ((ret = __os_seek(dbenv, fhp, 0, pagesize,
		    sizeof (u_int32_t), 0, DB_OS_SEEK_SET)) != 0 ||
		    (ret = __os_write(dbenv, fhp, &nextfree, sizeof(db_pgno_t),
		    &nio)) < 0 || nio == 0)
			goto err;
	}

	/* Write the buffer to the allocated page. */
	if ((ret = __os_io(dbenv, DB_IO_WRITE, fhp, newpgno, pagesize, 0,
	    pagesize, bhp->buf, &nio)) != 0 || nio == 0)
		goto err;

	/*
	 * Set up the frozen_bhp with the freezer page number.  The original
	 * buffer header is about to be freed, so transfer resources to the
	 * frozen header here.
	 */
#ifdef DIAG_MVCC
	memcpy(frozen_bhp, bhp, SSZ(BH, align_off));
#else
	memcpy(frozen_bhp, bhp, SSZ(BH, buf));
#endif
	frozen_bhp->priority = UINT32_MAX;
	--frozen_bhp->ref;
	F_SET(frozen_bhp, BH_FROZEN);
	F_CLR(frozen_bhp, BH_LOCKED);
	((BH_FROZEN_PAGE *)frozen_bhp)->spgno = newpgno;

	bhp->td_off = INVALID_ROFF;

	/*
	 * Add the frozen buffer to the version chain and update the hash
	 * bucket if this is the head revision.  __memp_alloc will remove it by
	 * calling __memp_bhfree on the old version of the buffer.
	 */
	SH_CHAIN_INSERT_AFTER(bhp, frozen_bhp, vc, __bh);
	if (SH_CHAIN_NEXT(frozen_bhp, vc, __bh) == NULL) {
		SH_TAILQ_INSERT_BEFORE(&hp->hash_bucket,
		    bhp, frozen_bhp, hq, __bh);
		SH_TAILQ_REMOVE(&hp->hash_bucket, bhp, hq, __bh);
	}

	/*
	 * Increment the file's block count -- freeing the original buffer will
	 * decrement it.
	 */
	++bh_mfp->block_cnt;

err:	if (real_name != NULL)
		__os_free(dbenv, real_name);
	if (fhp != NULL &&
	    (t_ret = __os_closehandle(dbenv, fhp)) != 0 && ret == 0)
		ret = t_ret;
	F_CLR(bhp, BH_LOCKED);
	--bhp->ref;

	/*
	 * If a thread of control is waiting on this buffer, wake it up.
	 */
	if (F_ISSET(hp, IO_WAITER)) {
		F_CLR(hp, IO_WAITER);
		MUTEX_UNLOCK(dbenv, hp->mtx_io);
	}
	return (ret);
}

static int
__pgno_cmp(a, b)
	const void *a, *b;
{
	db_pgno_t *ap, *bp;

	ap = (db_pgno_t *)a;
	bp = (db_pgno_t *)b;

	return (*bp - *ap);
}

/*
 * __memp_bh_freezefree --
 *	Free a buffer header in temporary storage.  This function should be
 *	called with the hash bucket locked.
 *
 * PUBLIC: int __memp_bh_freezefree __P((DB_MPOOL *, REGINFO *,
 * PUBLIC:	DB_MPOOL_HASH *, BH *, BH *));
 */
int
__memp_bh_freezefree(dbmp, infop, hp, frozen_bhp, alloc_bhp)
	DB_MPOOL *dbmp;
	REGINFO *infop;
	DB_MPOOL_HASH *hp;
	BH *frozen_bhp, *alloc_bhp;
{
	DB_ENV *dbenv;
	DB_FH *fhp;
	MPOOL *c_mp;
	MPOOLFILE *bh_mfp;
	db_pgno_t *freelist, freepgno, maxpgno, spgno;
	u_int32_t magic, nbucket, ncache, pagesize;
	int i, listsize, nfree, nio, ntrunc, ret, t_ret;
	char filename[100], *real_name;

	dbenv = dbmp->dbenv;
	fhp = NULL;
	c_mp = infop->primary;
	bh_mfp = R_ADDR(dbmp->reginfo, frozen_bhp->mf_offset);
	freelist = NULL;
	pagesize = bh_mfp->stat.st_pagesize;
	ret = 0;
	real_name = NULL;

	DB_ASSERT(dbenv, F_ISSET(frozen_bhp, BH_FROZEN));
	DB_ASSERT(dbenv, frozen_bhp->ref == 1);

	/*
	 * For now, keep things simple and have one file per page size per
	 * cache.  Concurrency will be suboptimal, but debugging should be
	 * simpler.
	 */
	ncache = (u_int32_t)(infop - dbmp->reginfo);
	nbucket = (u_int32_t)(hp - (DB_MPOOL_HASH *)R_ADDR(infop, c_mp->htab));
	snprintf(filename, sizeof(filename), "__db.freezer.%u.%u.%uK",
	    ncache, nbucket, pagesize / 1024);

	if ((ret = __db_appname(dbenv, DB_APP_NONE, filename, 0, NULL,
	    &real_name)) != 0)
		goto err;

	if ((ret = __os_open_extend(dbenv, real_name, pagesize, 0,
	    dbenv->db_mode, &fhp)) != 0)
		goto err;

	/*
	 * Read the first free page number -- we're about to free the page
	 * after we we read it.
	 */
	if ((ret = __os_read(dbenv, fhp, &magic, sizeof(u_int32_t),
	    &nio)) < 0 || nio == 0 ||
	    (ret = __os_read(dbenv, fhp, &freepgno, sizeof(db_pgno_t),
	    &nio)) < 0 || nio == 0 ||
	    (ret = __os_read(dbenv, fhp, &maxpgno, sizeof(db_pgno_t),
	    &nio)) < 0 || nio == 0)
		goto err;

	if (magic != DB_FREEZER_MAGIC) {
		ret = EINVAL;
		goto err;
	}

	spgno = ((BH_FROZEN_PAGE *)frozen_bhp)->spgno;

	/* Read the buffer from the frozen page. */
	if (alloc_bhp != NULL &&
	    ((ret = __os_io(dbenv, DB_IO_READ, fhp, spgno, pagesize,
	    0, pagesize, alloc_bhp->buf, &nio)) != 0 || nio == 0))
		goto err;

	/*
	 * Free the page from the file.  If it's the last page, truncate.
	 * Otherwise, update free page linked list.o
	 */
	if (spgno == maxpgno) {
		listsize = 100;
		nfree = 0;
		if ((ret = __os_malloc(dbenv,
		    listsize * sizeof (db_pgno_t), &freelist)) != 0)
			goto err;
		while (freepgno != 0) {
			if (nfree == listsize) {
				listsize *= 2;
				if ((ret = __os_realloc(dbenv,
				    listsize * sizeof (db_pgno_t),
				    &freelist)) != 0)
					goto err;
			}
			freelist[nfree++] = freepgno;
			if ((ret = __os_seek(dbenv, fhp, freepgno, pagesize,
			    0, 0, DB_OS_SEEK_SET)) != 0 ||
			    (ret = __os_read(dbenv, fhp, &freepgno,
			    sizeof(db_pgno_t), &nio)) < 0 || nio == 0)
				goto err;
		}
		qsort(freelist, nfree, sizeof (db_pgno_t), __pgno_cmp);
		for (ntrunc = 1; ntrunc < nfree; ntrunc++)
			if (freelist[nfree - ntrunc] !=
			    freelist[nfree + 1 - ntrunc] - 1)
				break;
		if (ntrunc == (int)maxpgno - 1) {
			ret = __os_closehandle(dbenv, fhp);
			fhp = NULL;
			if (ret != 0 ||
			    (ret = __os_unlink(dbenv, real_name)) != 0)
				goto err;
		} else {
#ifdef HAVE_FTRUNCATE
			if ((ret = __os_truncate(dbenv, fhp,
			    maxpgno - ntrunc, pagesize)) != 0)
				goto err;

			/* Fix up the linked list */
			freelist[nfree - ntrunc] = 0;
			if ((ret = __os_seek(dbenv, fhp, 0, pagesize,
			    sizeof (u_int32_t), 0, DB_OS_SEEK_SET)) != 0 ||
			    (ret = __os_write(dbenv, fhp, &freelist[0],
			    sizeof(db_pgno_t), &nio)) < 0 || nio == 0)
				goto err;
			for (i = 0; i < nfree - ntrunc; i++)
				if ((ret = __os_seek(dbenv, fhp, freelist[i],
				    pagesize, 0, 0, DB_OS_SEEK_SET)) != 0 ||
				    (ret = __os_write(dbenv, fhp,
				    &freelist[i + 1], sizeof(db_pgno_t),
				    &nio)) < 0 || nio == 0)
					goto err;
#endif
		}
	} else if ((ret = __os_seek(dbenv, fhp, spgno, pagesize, 0, 0,
	    DB_OS_SEEK_SET)) != 0 ||
	    (ret = __os_write(dbenv, fhp, &freepgno, sizeof(db_pgno_t),
	    &nio)) < 0 || nio == 0 ||
	    (ret = __os_seek(dbenv, fhp, 0, pagesize, sizeof(u_int32_t), 0,
	    DB_OS_SEEK_SET)) != 0 ||
	    (ret = __os_write(dbenv, fhp, &spgno, sizeof(db_pgno_t),
	    &nio)) < 0 || nio == 0)
		goto err;

	if (SH_CHAIN_NEXT(frozen_bhp, vc, __bh) == NULL) {
		if (SH_CHAIN_PREV(frozen_bhp, vc, __bh) != NULL)
			SH_TAILQ_INSERT_BEFORE(&hp->hash_bucket, frozen_bhp,
			    SH_CHAIN_PREV(frozen_bhp, vc, __bh), hq, __bh);
		SH_TAILQ_REMOVE(&hp->hash_bucket, frozen_bhp, hq, __bh);
	}
	SH_CHAIN_REMOVE(frozen_bhp, vc, __bh);

	MUTEX_UNLOCK(dbenv, hp->mtx_hash);
	MPOOL_REGION_LOCK(dbenv, infop);
	SH_TAILQ_INSERT_HEAD(&c_mp->free_frozen, frozen_bhp, hq, __bh);
	MPOOL_REGION_UNLOCK(dbenv, infop);
	MUTEX_LOCK(dbenv, hp->mtx_hash);

err:	if (real_name != NULL)
		__os_free(dbenv, real_name);
	if (freelist != NULL)
		__os_free(dbenv, freelist);
	if (fhp != NULL &&
	    (t_ret = __os_closehandle(dbenv, fhp)) != 0 && ret == 0)
		ret = t_ret;
	return (ret);
}

/*
 * __memp_bh_unfreeze --
 *	Restore a buffer from temporary storage when it is required by a
 *	snapshot transaction.
 *
 * PUBLIC: int __memp_bh_unfreeze __P((DB_MPOOL *, REGINFO *, DB_MPOOL_HASH *,
 * PUBLIC:     BH *, BH *));
 */
int
__memp_bh_unfreeze(dbmp, infop, hp, frozen_bhp, alloc_bhp)
	DB_MPOOL *dbmp;
	REGINFO *infop;
	DB_MPOOL_HASH *hp;
	BH *frozen_bhp;
	BH *alloc_bhp;
{
	DB_ENV *dbenv;
	int ret;

	dbenv = dbmp->dbenv;

#ifdef DIAG_MVCC
	memcpy(alloc_bhp, frozen_bhp, SSZ(BH, align_off));
#else
	memcpy(alloc_bhp, frozen_bhp, SSZ(BH, buf));
#endif
	F_CLR(alloc_bhp, BH_FROZEN);

	/*
	 * We need to drop the hash bucket mutex during the unfreeze, make sure
	 * that nobody reads the page mid-thaw.
	 */
	++alloc_bhp->ref;
	F_SET(alloc_bhp, BH_LOCKED);

	/*
	 * Add the unfrozen buffer to the version chain and update the hash
	 * bucket if this is the head revision.
	 */
	SH_CHAIN_INSERT_AFTER(frozen_bhp, alloc_bhp, vc, __bh);
	if (SH_CHAIN_NEXT(alloc_bhp, vc, __bh) == NULL) {
		SH_TAILQ_INSERT_BEFORE(&hp->hash_bucket,
		    frozen_bhp, alloc_bhp, hq, __bh);
		SH_TAILQ_REMOVE(&hp->hash_bucket, frozen_bhp, hq, __bh);
	}

	if ((ret = __memp_bh_freezefree(dbmp, infop, hp,
	    frozen_bhp, alloc_bhp)) != 0) {
		SH_CHAIN_REMOVE(alloc_bhp, vc, __bh);
		if (SH_CHAIN_NEXT(frozen_bhp, vc, __bh) == NULL) {
			SH_TAILQ_INSERT_BEFORE(&hp->hash_bucket,
			    alloc_bhp, frozen_bhp, hq, __bh);
			SH_TAILQ_REMOVE(&hp->hash_bucket, alloc_bhp, hq, __bh);
		}
		MUTEX_UNLOCK(dbenv, hp->mtx_hash);
		MPOOL_REGION_LOCK(dbenv, infop);
		__memp_free(infop,
		    (MPOOLFILE *)R_OFFSET(dbmp->reginfo, alloc_bhp->mf_offset),
		    alloc_bhp);
		MPOOL_REGION_UNLOCK(dbenv, infop);
		MUTEX_LOCK(dbenv, hp->mtx_hash);
	} else {
		F_CLR(alloc_bhp, BH_LOCKED);
		--alloc_bhp->ref;
	}

	/*
	 * If a thread of control is waiting on this buffer, wake it up.
	 */
	if (F_ISSET(hp, IO_WAITER)) {
		F_CLR(hp, IO_WAITER);
		MUTEX_UNLOCK(dbenv, hp->mtx_io);
	}

	return (ret);
}

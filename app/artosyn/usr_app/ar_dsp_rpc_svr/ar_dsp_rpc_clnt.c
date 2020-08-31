/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include <memory.h> /* for memset */
#include "ar_dsp_rpc.h"

/* Default timeout can be changed using clnt_control() */
static struct timeval TIMEOUT = { 25, 0 };

int *
ar_alg_cfg_dsp_1(AR_DSP_CFG_RPC_st *argp, CLIENT *clnt)
{
	static int clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, AR_ALG_CFG_DSP,
		(xdrproc_t) xdr_AR_DSP_CFG_RPC_st, (caddr_t) argp,
		(xdrproc_t) xdr_int, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

AR_DSP_ALG_RPC_OUT_st *
ar_do_algrithom_1(AR_DSP_ALG_RPC_IN_st *argp, CLIENT *clnt)
{
	static AR_DSP_ALG_RPC_OUT_st clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, AR_DO_ALGRITHOM,
		(xdrproc_t) xdr_AR_DSP_ALG_RPC_IN_st, (caddr_t) argp,
		(xdrproc_t) xdr_AR_DSP_ALG_RPC_OUT_st, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

AR_DSP_ALG_RPC_OUT_st *
ar_do_algrithom_with_timeout_1(AR_DSP_ALG_TO_RPC_IN_st *argp, CLIENT *clnt)
{
	static AR_DSP_ALG_RPC_OUT_st clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, AR_DO_ALGRITHOM_WITH_TIMEOUT,
		(xdrproc_t) xdr_AR_DSP_ALG_TO_RPC_IN_st, (caddr_t) argp,
		(xdrproc_t) xdr_AR_DSP_ALG_RPC_OUT_st, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

int *
ar_send_file_1(AR_FILE_CHUNK_st *argp, CLIENT *clnt)
{
	static int clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, AR_SEND_FILE,
		(xdrproc_t) xdr_AR_FILE_CHUNK_st, (caddr_t) argp,
		(xdrproc_t) xdr_int, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}
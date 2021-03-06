#include "crypto_alg.h"
#include "crypto_openssl.h"
#include "com_att_aro_pcap_AROCryptoAdapter.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

typedef unsigned char BYTE;

#define ARO_CRYPTO_ERROR -1
#define CTX_TSI_SERVER 0
#define CTX_TSI_CLIENT 1
#define CTX_TSI_PENDING 2

struct crypto_cipher* tsiserver_ctx_server  = NULL;
struct crypto_cipher* tsiserver_ctx_client  = NULL;
struct crypto_cipher* tsiclient_ctx_server  = NULL;
struct crypto_cipher* tsiclient_ctx_client  = NULL;
struct crypto_cipher* tsipending_ctx_server = NULL;
struct crypto_cipher* tsipending_ctx_client = NULL;
struct crypto_hash*   hmac = NULL;

void hexDump(char *  ba, int len){
	 printf("\n0x");
	for (int i = 0; i < len; i++)
	{
	    if(( i > 0 ) && ( i%4 == 0 )){
	    	 printf(" ");
	    }
	    printf("%02X", ba[i]);
	}
	printf("\n");
}
void hexDump(BYTE *  ba, int len){
	 printf("\n");
	for (int i = 0; i < len; i++)
	{
	    if(( i > 0 ) && ( i%4 == 0 )){
	    	 printf(" ");
	    }
	    printf("%02X", ba[i]);
	}
	printf("\n");
}

/*
 * ReadSSLKeys
 */
extern "C"
JNIEXPORT jint JNICALL Java_com_att_aro_core_securedpacketreader_CryptoAdapter_\
readSSLKeys(JNIEnv * env, jobject obj, jstring filename) {
    
    
    int ret = ARO_CRYPTO_ERROR;
    const char * szfilename = env->GetStringUTFChars(filename, 0);
    if (NULL == szfilename) {
        return ARO_CRYPTO_ERROR;
    }
    
    
    printf("\nkey file: %s\n", szfilename);
    

	short stringlength = 13;
    char *buffer = (char *) malloc(stringlength + 1);

    BYTE * tss = (BYTE * ) malloc(13);
    double ts = 0.0;
    int preMasterLen = 0;
    int masterLen = 48;
    BYTE * preMaster = (BYTE * ) malloc(256);
    BYTE * master = (BYTE * ) malloc(48);
    FILE * ifs = fopen(szfilename, "rb");

    bool vpnFormat = false;
    fread(buffer, sizeof(char), (size_t)stringlength, ifs);
	fseek(ifs , 0, SEEK_SET);
	if (buffer[0]==0x31){
		vpnFormat = true;
	}
	printf(vpnFormat?"vpn format yes":"vpn format no");

    size_t r = ARO_CRYPTO_ERROR;
    if (ifs != NULL) {
        while (!feof(ifs)) {

			if ( vpnFormat ){
				preMasterLen = 32;
				printf("\nVPN keys.ssl");
//				r = fread( & ts, 8, 1, ifs);
//				r = fread(tss, 8, 1, ifs);

//				fseek(ifs , 0, SEEK_SET);
				fread(buffer, sizeof(char), (size_t)stringlength, ifs);
				ts = atof(buffer)/1000; // nano to ms
				hexDump(buffer,13);
				printf("\nts :%lf",ts);

				r = fread(master, masterLen, 1, ifs); // A402BCB85EE37AA56C966BEFE54B2A85F07C0D3CCF962AC2AC9FAEBE2D944B875EC9AF7C79CE0C830FAB11FBFDE90BE7
				printf("\nmaster:");
				hexDump(master,48);

				r = fread(preMaster, preMasterLen, 1, ifs); // 577FEEA20450F1F776EC9FF835F89FB085F19BF01BD607C7A611CA922437B619
				printf("preMaster1:");
				hexDump(preMaster,preMasterLen);

				r = fread(preMaster, preMasterLen, 1, ifs); // 577FEEA3F3430549FF3D8763811BA0CF36DDFF7F9FC5EF6DF0F812D7639606FD
				printf("preMaster2:");
				hexDump(preMaster,preMasterLen);

				hexDump(tss,4);
//				exit(0);
        	}
			else
			{
				printf("\nrooted keys.ssl");
				r = fread( &ts, 8, 1, ifs);
				r = fread( &preMasterLen, 4, 1, ifs);
				r = fread(preMaster, preMasterLen, 1, ifs);
				r = fread(master, masterLen, 1, ifs);


			printf("\nts :%lf",ts);
			printf("\npreMasterLen :%i",preMasterLen);
			printf("\nmasterLen :%i",masterLen);
			printf("\npreMaster:"); hexDump(preMaster,preMasterLen);
			printf("\nmaster:");    hexDump(master,masterLen);

//			exit(0);
        	}
            jclass cls;
            jmethodID javaCallBack;
            jbyteArray preMasterArray;
            jbyteArray masterArray;
            cls = env->GetObjectClass(obj);
            javaCallBack = env->GetMethodID(cls, "sslKeyHandler", "(DI[B[B)V");

            if (javaCallBack != NULL) {

                preMasterArray = env->NewByteArray((jsize) preMasterLen);
                env->SetByteArrayRegion(preMasterArray, 0, (jsize) preMasterLen, (const jbyte * ) preMaster);

                masterArray = env->NewByteArray((jsize) 48);
                env->SetByteArrayRegion(masterArray, 0, (jsize) 48, (const jbyte * ) master);

                // send to the callback method
                env->CallVoidMethod(obj, javaCallBack, (jdouble) ts, (jint) preMasterLen, preMasterArray, masterArray);

                env->DeleteLocalRef(preMasterArray); // available for garbage collection
                env->DeleteLocalRef(masterArray);    // available for garbage collection
            }
        }
        fclose(ifs);
        ret = 0;
    }
    
    env->ReleaseStringUTFChars(filename, szfilename);
    return ret;
}


/*
 * crypto-hash-Init-Update-Finish-1
 */
extern "C"
JNIEXPORT jint JNICALL Java_com_att_aro_core_securedpacketreader_CryptoAdapter_\
cryptohashInitUpdateFinish1(JNIEnv * env, jobject obj, jint dir) {

	int test = (int) dir;
	int ret = crypto_hash_finish1(test);
	return ret;

}

/*
 * crypto-cipher-decrypt
 *
 * cryptocipherdecrypt
 *
 * jint			pCipher		cipher index
 * jbyteArray	enc			encoded data
 * jbyteArray	plain       buffer to receive decoded data
 * jint			enclength   length of encoded data
 * jint			objectType  CTX_TSI_SERVER, CTX_TSI_CLIENT, CTX_TSI_PENDING
 *
 * This method should be called ONLY by tsiServer or tsiClient TLS_SESSION_INFO
 */
extern "C"
JNIEXPORT jint JNICALL Java_com_att_aro_core_securedpacketreader_CryptoAdapter_\
cryptocipherdecrypt(JNIEnv * env, jobject obj, jint pCipher, jbyteArray enc, jbyteArray plain, jint enclength, jint objectType) {

    int ret = ARO_CRYPTO_ERROR;
    int i_pCipher = (int) pCipher;
    int i_enclength = (int) enclength;
    int i_objectType = (int) objectType;

    jbyte * encbuff = NULL;
    encbuff = env->GetByteArrayElements(enc, NULL);

    jbyte * plainbuff = NULL;
    plainbuff = env->GetByteArrayElements(plain, NULL);

    if (!encbuff || !plainbuff) {
        return NULL;
    }

    struct crypto_cipher * ctx = NULL;

    if (i_pCipher == CTX_TSI_CLIENT) {
        if (i_objectType == CTX_TSI_CLIENT) {
            ctx = tsiclient_ctx_client;
        } else {
            ctx = tsiserver_ctx_client;
        }
    } else {
        if (i_objectType == CTX_TSI_CLIENT) {
            ctx = tsiclient_ctx_server;
        } else {
            ctx = tsiserver_ctx_server;
        }
    }

    ret = crypto_cipher_decrypt(ctx, (BYTE * ) encbuff, (BYTE * ) plainbuff, i_enclength);

    env->ReleaseByteArrayElements(enc, encbuff, 0);
    env->ReleaseByteArrayElements(plain, plainbuff, 0);

    return ret;
}

/*
 * crypto-cipher-init
 *
 * This method should be called ONLY by tsiPending TLS_SESSION_INFO
 */
extern "C"
JNIEXPORT jint JNICALL Java_com_att_aro_core_securedpacketreader_CryptoAdapter_\
cryptocipherinit(JNIEnv * env, jobject obj, jint alg, jbyteArray temp1, jbyteArray temp2, jint key_material, jint bClient) {

    int ret = ARO_CRYPTO_ERROR;

    int i_alg = (int) alg;
    enum crypto_cipher_alg e_alg = (enum crypto_cipher_alg) i_alg;

    jbyte * temp1buff = NULL;
    temp1buff = env->GetByteArrayElements(temp1, NULL);

    jbyte * temp2buff = NULL;
    temp2buff = env->GetByteArrayElements(temp2, NULL);

    if (!temp1buff || !temp2buff) {
        return ret;
    }

    int i_key_material = (int) key_material;
    int i_bClient = (int) bClient;

    struct crypto_cipher * ctx = NULL;
    ctx = crypto_cipher_init(e_alg, (const BYTE * ) temp1buff, (const BYTE * ) temp2buff, i_key_material);
    env->ReleaseByteArrayElements(temp1, temp1buff, 0);
    env->ReleaseByteArrayElements(temp2, temp2buff, 0);
    if (ctx == NULL) {
        return ret;
    }

    if (i_bClient == 1) {
        tsipending_ctx_client = ctx;
    } else {
        tsipending_ctx_server = ctx;
    }
    ret = 0;

    return ret;
}

/*
 * crypto-cipher-deinit
 */
extern "C"
JNIEXPORT void JNICALL Java_com_att_aro_core_securedpacketreader_CryptoAdapter_\
cryptocipherdeinit(JNIEnv * env, jobject obj, jint objectType) {

    int i_objectType = (int) objectType;
    if (i_objectType == CTX_TSI_SERVER) {
        if (tsiserver_ctx_client) {
            crypto_cipher_deinit(tsiserver_ctx_client);
            tsiserver_ctx_client = NULL;
        }
        if (tsiserver_ctx_server) {
            crypto_cipher_deinit(tsiserver_ctx_server);
            tsiserver_ctx_server = NULL;
        }
    } else if (i_objectType == CTX_TSI_CLIENT) {
        if (tsiclient_ctx_client) {
            crypto_cipher_deinit(tsiclient_ctx_client);
            tsiclient_ctx_client = NULL;
        }
        if (tsiclient_ctx_server) {
            crypto_cipher_deinit(tsiclient_ctx_server);
            tsiclient_ctx_server = NULL;
        }
    } else if (i_objectType == CTX_TSI_PENDING) {
        if (tsipending_ctx_client) {
            crypto_cipher_deinit(tsipending_ctx_client);
            tsipending_ctx_client = NULL;
        }
        if (tsipending_ctx_server) {
            crypto_cipher_deinit(tsipending_ctx_server);
            tsipending_ctx_server = NULL;
        }
    }
}

/*
 * set-crypto-cipher-null
 */
extern "C"
JNIEXPORT void JNICALL Java_com_att_aro_core_securedpacketreader_CryptoAdapter_\
setcryptociphernull(JNIEnv * env, jobject obj, jint objectType, jint bClient) {

    int i_objectType = (int) objectType;
    if (i_objectType == CTX_TSI_SERVER) {
        if (bClient == CTX_TSI_CLIENT) {
            tsiserver_ctx_client = NULL;
        } else {
            tsiserver_ctx_server = NULL;
        }

    } else if (i_objectType == CTX_TSI_CLIENT) {
        if (bClient == CTX_TSI_CLIENT) {
            tsiclient_ctx_client = NULL;
        } else {
            tsiclient_ctx_server = NULL;
        }
    } else if (i_objectType == CTX_TSI_PENDING) {
        if (bClient == CTX_TSI_CLIENT) {
            tsipending_ctx_client = NULL;
        } else {
            tsipending_ctx_server = NULL;
        }
    }
}

/*
 * copy-crypto-cipher
 */
extern "C"
JNIEXPORT void JNICALL Java_com_att_aro_core_securedpacketreader_CryptoAdapter_\
copycryptocipher(JNIEnv * env, jobject obj, jint from, jint to) {

    int i_from = (int) from;
    int i_to = (int) to;
    if (i_from == CTX_TSI_SERVER) {
        if (i_to == CTX_TSI_CLIENT) {
            tsiclient_ctx_server = tsiserver_ctx_server;
            tsiclient_ctx_client = tsiserver_ctx_client;
        } else {
            tsipending_ctx_server = tsiserver_ctx_server;
            tsipending_ctx_client = tsiserver_ctx_client;
        }
    }

    if (i_from == CTX_TSI_CLIENT) {
        if (i_to == CTX_TSI_SERVER) {
            tsiserver_ctx_server = tsiclient_ctx_server;
            tsiserver_ctx_client = tsiclient_ctx_client;
        } else {
            tsipending_ctx_server = tsiclient_ctx_server;
            tsipending_ctx_client = tsiclient_ctx_client;
        }
    }

    if (i_from == CTX_TSI_PENDING) {
        if (i_to == CTX_TSI_SERVER) {
            tsiserver_ctx_server = tsipending_ctx_server;
            tsiserver_ctx_client = tsipending_ctx_client;
        } else {
            tsiclient_ctx_server = tsipending_ctx_server;
            tsiclient_ctx_client = tsipending_ctx_client;
        }
    }
}

//

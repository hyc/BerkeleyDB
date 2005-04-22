/*
 * Automatically generated by jrpcgen 0.95.1 on 3/26/03 6:40 PM
 * jrpcgen is part of the "Remote Tea" ONC/RPC package for Java
 * See http://acplt.org/ks/remotetea.html for details
 */
package com.sleepycat.db.rpcserver;
import org.acplt.oncrpc.*;
import java.io.IOException;

public class __db_get_re_delim_reply implements XdrAble {
    public int status;
    public int delim;

    public __db_get_re_delim_reply() {
    }

    public __db_get_re_delim_reply(XdrDecodingStream xdr)
           throws OncRpcException, IOException {
        xdrDecode(xdr);
    }

    public void xdrEncode(XdrEncodingStream xdr)
           throws OncRpcException, IOException {
        xdr.xdrEncodeInt(status);
        xdr.xdrEncodeInt(delim);
    }

    public void xdrDecode(XdrDecodingStream xdr)
           throws OncRpcException, IOException {
        status = xdr.xdrDecodeInt();
        delim = xdr.xdrDecodeInt();
    }

}
// End of __db_get_re_delim_reply.java

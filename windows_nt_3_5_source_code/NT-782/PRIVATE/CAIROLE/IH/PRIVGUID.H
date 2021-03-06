/* this file is the master definition of all OLE2 product GUIDs (public and
   private).  All GUIDs used by the ole2 product are of the form:

	   xxxxxxxx-xxxx-xxxY-C000-000000000046

   This range is broken down as follows:

	   000000xx-0000-0000-C000-000000000046 compobj IIDs 
	   000001xx-0000-0000-C000-000000000046 ole2 IIDs
	   000002xx-0000-0000-C000-000000000046 16bit ole2 smoke test
	   000003xx-0000-0000-C000-000000000046 ole2 CLSIDs 
	   000004xx-0000-0000-C000-000000000046 ole2 sample apps (see DouglasH)

   Other interesting ranges are as follows:

	   0003xxxx-0000-0000-C000-000000000046 ole1 CLSIDs (ole1cls.h)
	   0004xxxx-0000-0000-C000-000000000046 hashed ole1 CLSIDs

*/

////////////////////////////////////////////////////////////////////////////
// Range 0x000 - 0x0ff : compobj IIDs - 
//             IID_IUnknown,            0x00000000L
//             IID_IClassFactory,       0x00000001L
//             IID_IMalloc,             0x00000002L
//             IID_IMarshal,            0x00000003L

//             IID_ILockBytes,          0x0000000aL
//             IID_IStorage,            0x0000000bL
//             IID_IStream,             0x0000000cL
//             IID_IEnumSTATSTG,        0x0000000dL

//             IID_IBindCtx,            0x0000000eL
//             IID_IMoniker,            0x0000000fL
//             IID_IRunningObjectTable, 0x00000010L
//             IID_IInternalMoniker,    0x00000011L

//             IID_IRootStorage,        0x00000012L
//             IID_IDfReserved1,        0x00000013L
//             IID_IDfReserved2,        0x00000014L
//             IID_IDfReserved3,        0x00000015L

//             IID_IMessageFilter,      0x00000016L

//             CLSID_StdMarshal,        0x00000017L

//             IID_IStdMarshalInfo,     0x00000018L

//             IID_IExternalConnection, 0x00000019L

//             IID_IWeakRef,		0x0000001aL

/* interface for subpieces of standard remoting */
DEFINE_OLEGUID(IID_IStdIdentity,	0x0000001bL, 0, 0);
DEFINE_OLEGUID(IID_IRemoteHdlr,		0x0000001cL, 0, 0);

/* NOTE: LSB values 0x1d through 0xff are unused */


////////////////////////////////////////////////////////////////////////////
// Range 0x100 - 0x1ff : upper layer IIDs - 
//             IID_IEnumUnknown,	0x00000100L
//             IID_IEnumString,         0x00000101L
//             IID_IEnumMoniker,        0x00000102L
//             IID_IEnumFORMATETC,      0x00000103L
//             IID_IEnumOLEVERB,        0x00000104L
//             IID_IEnumSTATDATA,       0x00000105L

//             IID_IEnumGeneric,        0x00000106L
//             IID_IEnumHolder,         0x00000107L
//             IID_IEnumCallback,       0x00000108L

//             IID_IPersistStream,      0x00000109L
//             IID_IPersistStorage,     0x0000010aL
//             IID_IPersistFile,        0x0000010bL
//             IID_IPersist,            0x0000010cL

//             IID_IViewObject,         0x0000010dL
//             IID_IDataObject,         0x0000010eL
//             IID_IAdviseSink,         0x0000010fL
//             IID_IDataAdviseHolder,   0x00000110L
//             IID_IOleAdviseHolder,    0x00000111L

//             IID_IOleObject,          0x00000112L
//             IID_IOleInPlaceObject,   0x00000113L
//             IID_IOleWindow,          0x00000114L
//             IID_IOleInPlaceUIWindow, 0x00000115L
//             IID_IOleInPlaceFrame,    0x00000116L
//             IID_IOleInPlaceActiveObject, 0x00000117L

//             IID_IOleClientSite,      0x00000118L
//             IID_IOleInPlaceSite,     0x00000119L

//             IID_IParseDisplayName,   0x0000011aL
//             IID_IOleContainer,       0x0000011bL
//             IID_IOleItemContainer,   0x0000011cL

//             IID_IOleLink,            0x0000011dL
//             IID_IOleCache,           0x0000011eL
//             IID_IOleManager,         0x0000011fL
//             IID_IOlePresObj,         0x00000120L

//             IID_IDropSource,         0x00000121L
//             IID_IDropTarget,         0x00000122L

//             IID_IDebug,              0x00000123L
//             IID_IDebugStream,        0x00000124L

//             IID_IAdviseSink2,        0x00000125L

//             IID_IRunnableObject,     0x00000126L

//             IID_IViewObject2,	0x00000127L
//             IID_IOleCache2,		0x00000128L
//             IID_IOleCacheControl,    0x00000129L

/* NOTE: LSB values 0x2a through 0xff are unused */


////////////////////////////////////////////////////////////////////////////
// Range 0x300 - 0x3ff : internal CLSIDs
DEFINE_OLEGUID(CLSID_StdOleLink,	0x00000300, 0, 0);
DEFINE_OLEGUID(CLSID_StdMemStm,		0x00000301, 0, 0);
DEFINE_OLEGUID(CLSID_StdMemBytes,	0x00000302, 0, 0);
DEFINE_OLEGUID(CLSID_FileMoniker,	0x00000303, 0, 0);
DEFINE_OLEGUID(CLSID_ItemMoniker,	0x00000304, 0, 0);
DEFINE_OLEGUID(CLSID_AntiMoniker,	0x00000305, 0, 0);
DEFINE_OLEGUID(CLSID_PointerMoniker,	0x00000306, 0, 0);
// NOT TO BE USED			0x00000307, 0, 0);
DEFINE_OLEGUID(CLSID_PackagerMoniker,	0x00000308, 0, 0);
DEFINE_OLEGUID(CLSID_CompositeMoniker,	0x00000309, 0, 0);
// NOT TO BE USED		        0x0000030a, 0, 0);
// NOT TO BE USED (was CLSID_DfMarshal) 0x0000030b, 0, 0);

// NOT TO BE USED 0x30c - 0x315 - old PS CLSID's

//             CLSID_Picture_Metafile,  0x00000315
//             CLSID_Picture_Dib,       0x00000316

DEFINE_OLEGUID(CLSID_RemoteHdlr,        0x00000317, 0, 0);
DEFINE_OLEGUID(CLSID_RpcChannelBuffer,  0x00000318, 0, 0);

//             CLSID_Picture_EnhMetafile,0x00000319

/* NOTE: LSB values 0x1a through 0xff are unused */


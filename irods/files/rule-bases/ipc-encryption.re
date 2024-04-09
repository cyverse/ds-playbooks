# Encryption enforcement policy


# Checks if encryption is required for the collection entity
_ipcIsEncryptionRequired(*Coll) {
  *isRequired = false;
  *res = SELECT META_COLL_ATTR_VALUE WHERE COLL_NAME == *Coll AND META_COLL_ATTR_NAME == 'encryption.required';
  foreach (*record in *res) {
    *isRequired = bool(*record.META_COLL_ATTR_VALUE);
    break;
  }
  *isRequired;
}


# This rule checks if encryption is required
# if encryption is required, reject creating non-encrypted files
_ipcEncryptionCheckEncryptionRequired(*Path) {
    msiSplitPath(*Path, *parentColl, *objName);
    # check if parent coll has encryption.required avu
    if (_ipcIsEncryptionRequired(*parentColl)) {
        # all encrypted files will have ".enc" extension
        if (!_ipc_endsWith(*objName, ".enc")) {
            # fail to prevent iRODS from creating the file without encryption
            writeLine('serverLog', "Failed to create data object, encryption is required under *parentColl");
            cut;
            failmsg(-815000, 'CYVERSE ERROR:  attempt to create unencrypted data object');
        }
    }
}

_ipcEncryptionRejectBulkRegIfEncryptionRequired(*Path) {
    msiSplitPath(*Path, *parentColl, *objName);
    if (_ipcIsEncryptionRequired(*parentColl)) {
        # we don't allow bulk registering files 
        writeLine('serverLog', "Failed to bulk register data objects in encryption required collection *parentColl");
        cut;
        failmsg(-815000, 'CYVERSE ERROR:  attempt to bulk registering data objects in encryption required collection'); 
    }
}

_ipcEncryptionCopyAVUFromParent(*Path) {
    msiSplitPath(*Path, *parentColl, *collName);
    if (_ipcIsEncryptionRequired(*parentColl)) {
        # Add encryption require meta to the new coll
        *err = errormsg(msiModAVUMetadata("-C", *Path, 'set', 'encryption.required', "true", ''), *msg);
        if (*err < 0) { writeLine('serverLog', *msg); }

        # Add encryption mode meta to the new coll
        *res = SELECT META_COLL_ATTR_VALUE WHERE COLL_NAME == *parentColl AND META_COLL_ATTR_NAME == 'encryption.mode';
        foreach (*record in *res) {
            *err = errormsg(msiModAVUMetadata("-C", *Path, 'set', 'encryption.mode', *record.META_COLL_ATTR_VALUE, ''), *msg);
            if (*err < 0) { writeLine('serverLog', *msg); }
            break;
        }
    }
}

ipcEncryption_api_data_obj_create_pre(*Instance, *Comm, *DataObjInp) {
    _ipcEncryptionCheckEncryptionRequired(*DataObjInp.obj_path);
}

ipcEncryption_api_data_obj_open_pre(*Instance, *Comm, *DataObjInp) {
    _ipcEncryptionCheckEncryptionRequired(*DataObjInp.obj_path);
}

ipcEncryption_api_data_obj_put_pre(*Instance, *Comm, *DataObjInp) {
    _ipcEncryptionCheckEncryptionRequired(*DataObjInp.obj_path);
}

ipcEncryption_api_data_obj_copy_pre(*Instance, *Comm, *DataObjCopyInp) {
    _ipcEncryptionCheckEncryptionRequired(*DataObjCopyInp.dst_obj_path);
}

ipcEncryption_api_data_obj_rename_pre(*Instance, *Comm, *DataObjRenameInp) {
    _ipcEncryptionCheckEncryptionRequired(*DataObjRenameInp.dst_obj_path);
}

ipcEncryption_api_struct_file_ext_and_reg_pre(*Instance, *Comm, *StructFileExtAndRegInp) {
    _ipcEncryptionRejectBulkRegIfEncryptionRequired(StructFileExtAndRegInp.obj_path)
}

ipcEncryption_api_coll_create_post(*Instance, *Comm, *CollCreateInp) {
    _ipcEncryptionCopyAVUFromParent(*CollCreateInp.coll_name)
}
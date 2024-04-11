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


# This rule checks if encryption is required and reject creating non-encrypted files
_ipcEncryptionCheckEncryptionRequiredForDataObj(*Path) {
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

_ipcEncryptionCheckEncryptionRequiredForCollInternal(*Coll) {
    # check if src coll has non-encrypted data objects
    *res = SELECT COLL_NAME, DATA_NAME WHERE COLL_NAME == *Coll;
    foreach (*record in *res) {
        # all encrypted files will have ".enc" extension
        if (!_ipc_endsWith(*record.DATA_NAME, ".enc")) {
            # fail to prevent iRODS from creating the file without encryption
            writeLine('serverLog', "Failed to create data object, encryption is required under *Coll");
            cut;
            failmsg(-815000, 'CYVERSE ERROR:  attempt to create unencrypted data object');
        }
    }

    *res = SELECT COLL_NAME WHERE COLL_PARENT_NAME == *Coll;
    foreach (*record in *res) {
        # run recursively
        # this might be very expensive if the directory tree is very deep

        _ipcEncryptionCheckEncryptionRequiredForCollInternal(*record.COLL_NAME);
    }
}

# This rule checks if encryption is required and reject creating collections containing non-encrypted files
_ipcEncryptionCheckEncryptionRequiredForColl(*SrcColl, *DstColl) {
    msiSplitPath(*DstColl, *parentColl, *collName);
    # check if parent coll has encryption.required avu
    if (_ipcIsEncryptionRequired(*parentColl)) {
        _ipcEncryptionCheckEncryptionRequiredForCollInternal(*SrcColl);
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


_ipcEncryptionCopyAVUFromParentInternal(*Coll, *EncryptionMode) {
    # Add encryption require meta to the sub coll
    *err = errormsg(msiModAVUMetadata("-C", *Coll, 'set', 'encryption.required', "true", ''), *msg);
    if (*err < 0) { writeLine('serverLog', *msg); }

    if (*EncryptionMode != '') {
        *err = errormsg(msiModAVUMetadata("-C", *Coll, 'set', 'encryption.mode', *EncryptionMode, ''), *msg);
        if (*err < 0) { writeLine('serverLog', *msg); }  
    }

    *res = SELECT COLL_NAME WHERE COLL_PARENT_NAME == *Coll;
    foreach (*record in *res) {
        # run recursively
        # this might be very expensive if the directory tree is very deep

        _ipcEncryptionCopyAVUFromParentInternal(*record.COLL_NAME, *EncryptionMode);
    }
}

_ipcEncryptionCopyAVUFromParent(*Path) {
    msiSplitPath(*Path, *parentColl, *collName);
    if (_ipcIsEncryptionRequired(*parentColl)) {
        *mode = ''
        *res = SELECT META_COLL_ATTR_VALUE WHERE COLL_NAME == *parentColl AND META_COLL_ATTR_NAME == 'encryption.mode';
        foreach (*record in *res) {
            *mode = *record.META_COLL_ATTR_VALUE;
            break;
        }

        _ipcEncryptionCopyAVUFromParentInternal(*Path, *mode);
    }
}

ipcEncryption_api_data_obj_create_pre(*Instance, *Comm, *DataObjInp) {
    _ipcEncryptionCheckEncryptionRequiredForDataObj(*DataObjInp.obj_path);
}

ipcEncryption_api_data_obj_open_pre(*Instance, *Comm, *DataObjInp) {
    _ipcEncryptionCheckEncryptionRequiredForDataObj(*DataObjInp.obj_path);
}

ipcEncryption_api_data_obj_put_pre(*Instance, *Comm, *DataObjInp) {
    _ipcEncryptionCheckEncryptionRequiredForDataObj(*DataObjInp.obj_path);
}

ipcEncryption_api_data_obj_copy_pre(*Instance, *Comm, *DataObjCopyInp) {
    _ipcEncryptionCheckEncryptionRequiredForDataObj(*DataObjCopyInp.dst_obj_path);
}

ipcEncryption_api_data_obj_rename_pre(*Instance, *Comm, *DataObjRenameInp) {
    if (int(*DataObjRenameInp.src_opr_type) == 11) {
        # data object
        _ipcEncryptionCheckEncryptionRequiredForDataObj(*DataObjRenameInp.dst_obj_path);
    } else if (int(*DataObjRenameInp.src_opr_type) == 12) {
        # collection
        _ipcEncryptionCheckEncryptionRequiredForColl(*DataObjRenameInp.src_obj_path, *DataObjRenameInp.dst_obj_path);
    }
}

ipcEncryption_api_data_obj_rename_post(*Instance, *Comm, *DataObjRenameInp) {
    if (int(*DataObjRenameInp.src_opr_type) == 12) {
        # collection
        _ipcEncryptionCopyAVUFromParent(*DataObjRenameInp.dst_obj_path);
    }
}

ipcEncryption_api_struct_file_ext_and_reg_pre(*Instance, *Comm, *StructFileExtAndRegInp) {
    _ipcEncryptionRejectBulkRegIfEncryptionRequired(StructFileExtAndRegInp.obj_path);
}

ipcEncryption_api_coll_create_post(*Instance, *Comm, *CollCreateInp) {
    _ipcEncryptionCopyAVUFromParent(*CollCreateInp.coll_name);
}
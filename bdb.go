package berkeleydb

// #cgo LDFLAGS: -ldb
// #include <stdlib.h>
// #include "bdb.h"
import "C"
import "unsafe"
import "sync"

// operation status
type Status int
const (
	BDB_OK Status = C.BDB_OK                 // Its alright
	BDB_NOT_EXISTS Status = C.BDB_NOT_EXISTS // DB does not exists or do not have access
	BDB_TEMP_ERROR Status = C.BDB_TEMP_ERROR // Temporary error
	BDB_ERROR Status = C.BDB_ERROR           // Unrecoverable error
	BDB_FORMAT Status = C.BDB_FORMAT         // Do not reconize the database format
)

type BDB struct {
	bdb *C.bdb
	mux sync.Mutex
}

// Open DB file. Return nil if an error occurs
func Open(file string)(*BDB) {
	var bdb *BDB
	var cfile *C.char

	cfile = C.CString(file)
	defer C.free(unsafe.Pointer(cfile))
	bdb = &BDB{}
	bdb.bdb = C.bdb_open(cfile);
	if bdb.bdb == nil {
		return nil
	}
	return bdb
}

// Get key in the DB, return BDB_OK on suces, otherwise check Status documentation
func (bdb *BDB)Get(key string)(string, Status) {
	var ckey *C.char
	var cvalue *C.char
	var value string
	var ret C.int
	var status Status

	bdb.mux.Lock()
	ckey = C.CString(key)
	cvalue = C.bdb_get(bdb.bdb, ckey)
	value = C.GoString(cvalue)
	C.free(unsafe.Pointer(ckey))
	C.free(unsafe.Pointer(cvalue))
	ret = C.bdb_status(bdb.bdb)
	status = Status(ret)
	bdb.mux.Unlock()

	return value, status
}

// Close DB handle
func (bdb *BDB)Close()() {
	C.bdb_close(bdb.bdb)
	bdb.bdb = nil
}

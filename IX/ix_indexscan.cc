#include "ix_internal.h"

IX_IndexScan::IX_IndexScan() {
}

IX_IndexScan::~IX_IndexScan() {
}

RC IX_IndexScan::OpenScan(const IX_IndexHandle &indexHandle, CompOp compOp, void *value, ClientHint pinHint) {
}

RC IX_IndexScan::GetNextEntry(RID &rid) {
}

RC IX_IndexScan::CloseScan() {
}

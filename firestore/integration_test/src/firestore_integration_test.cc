#include "firestore_integration_test.h"

#include <cstdlib>
#include <fstream>
#include <sstream>

#include "absl/strings/ascii.h"
#include "app_framework.h"
#include "util/autoid.h"
#include "util/global_state.h"

namespace firebase {
namespace firestore {
namespace testing {

namespace {
// name of FirebaseApp to use for bootstrapping data into Firestore. We use a
// non-default app to avoid data ending up in the cache before tests run.
static const char* kBootstrapAppName = "bootstrap";

// Set Firestore up to use Firestore Emulator if it can be found.
void LocateEmulator(Firestore* db) {
  // iOS and Android pass emulator address differently, iOS writes it to a
  // temp file, but there is no equivalent to `/tmp/` for Android, so it
  // uses an environment variable instead.
  // TODO(wuandy): See if we can use environment variable for iOS as well?
  std::ifstream ifs("/tmp/emulator_address");
  std::stringstream buffer;
  buffer << ifs.rdbuf();
  std::string address;
  if (ifs.good()) {
    address = buffer.str();
  } else if (std::getenv("FIRESTORE_EMULATOR_HOST")) {
    address = std::getenv("FIRESTORE_EMULATOR_HOST");
  }

#if !defined(__ANDROID__)
  absl::StripAsciiWhitespace(&address);
#endif  // !defined(__ANDROID__)
  if (!address.empty()) {
    auto settings = db->settings();
    settings.set_host(address);
    // Emulator does not support ssl yet.
    settings.set_ssl_enabled(false);
    db->set_settings(settings);
  }
}

}  // namespace

std::string ToFirestoreErrorCodeName(int error_code) {
  switch (error_code) {
    case kErrorOk:
      return "kErrorOk";
    case kErrorCancelled:
      return "kErrorCancelled";
    case kErrorUnknown:
      return "kErrorUnknown";
    case kErrorInvalidArgument:
      return "kErrorInvalidArgument";
    case kErrorDeadlineExceeded:
      return "kErrorDeadlineExceeded";
    case kErrorNotFound:
      return "kErrorNotFound";
    case kErrorAlreadyExists:
      return "kErrorAlreadyExists";
    case kErrorPermissionDenied:
      return "kErrorPermissionDenied";
    case kErrorResourceExhausted:
      return "kErrorResourceExhausted";
    case kErrorFailedPrecondition:
      return "kErrorFailedPrecondition";
    case kErrorAborted:
      return "kErrorAborted";
    case kErrorOutOfRange:
      return "kErrorOutOfRange";
    case kErrorUnimplemented:
      return "kErrorUnimplemented";
    case kErrorInternal:
      return "kErrorInternal";
    case kErrorUnavailable:
      return "kErrorUnavailable";
    case kErrorDataLoss:
      return "kErrorDataLoss";
    case kErrorUnauthenticated:
      return "kErrorUnauthenticated";
    default:
      return "[invalid error code]";
  }
}

int WaitFor(const FutureBase& future) {
  // Instead of getting a clock, we count the cycles instead.
  int cycles = kTimeOutMillis / kCheckIntervalMillis;
  while (future.status() == FutureStatus::kFutureStatusPending && cycles > 0) {
    if (ProcessEvents(kCheckIntervalMillis)) {
      std::cout << "WARNING: app receives an event requesting exit."
                << std::endl;
      break;
    }
    --cycles;
  }
  return cycles;
}

void FirestoreIntegrationTest::SetUp() {
  FirebaseTest::SetUp();
  firestore_factory_ = FirestoreTestingGlobalState::GetInstance().CreateFirestoreFactory();
}

void FirestoreIntegrationTest::TearDown() {
  firestore_factory_.reset();
  FirebaseTest::TearDown();
}

App* FirestoreIntegrationTest::app() {
  return firestore_factory_->app_factory().GetDefaultInstance();
}

Firestore* FirestoreIntegrationTest::TestFirestore() const {
  Firestore* db = firestore_factory_->GetDefaultInstance();
  LocateEmulator(db);
  return db;
}
Firestore* FirestoreIntegrationTest::TestFirestore(const std::string& name) const {
  Firestore* db = firestore_factory_->GetInstance(name);
  LocateEmulator(db);
  return db;
}

void FirestoreIntegrationTest::DeleteFirestore(Firestore* firestore) {
  firestore_factory_->Delete(firestore);
}

void FirestoreIntegrationTest::DisownFirestore(Firestore* firestore) {
  firestore_factory_->Disown(firestore);
}

CollectionReference FirestoreIntegrationTest::Collection() const {
  return TestFirestore()->Collection(CreateAutoId());
}

CollectionReference FirestoreIntegrationTest::Collection(
    const std::string& name_prefix) const {
  return TestFirestore()->Collection(name_prefix + "_" + CreateAutoId());
}

CollectionReference FirestoreIntegrationTest::Collection(
    const std::map<std::string, MapFieldValue>& docs) const {
  CollectionReference result = Collection();
  WriteDocuments(TestFirestore(kBootstrapAppName)->Collection(result.path()),
                 docs);
  return result;
}

std::string FirestoreIntegrationTest::DocumentPath() const {
  return "test-collection/" + CreateAutoId();
}

DocumentReference FirestoreIntegrationTest::Document() const {
  return TestFirestore()->Document(DocumentPath());
}

DocumentReference FirestoreIntegrationTest::DocumentWithData(
    const MapFieldValue& data) const {
  DocumentReference docRef = Document();
  WriteDocument(docRef, data);
  return docRef;
}

void FirestoreIntegrationTest::WriteDocument(DocumentReference reference,
                                             const MapFieldValue& data) const {
  Future<void> future = reference.Set(data);
  Await(future);
  FailIfUnsuccessful("WriteDocument", future);
}

void FirestoreIntegrationTest::WriteDocuments(
    CollectionReference reference,
    const std::map<std::string, MapFieldValue>& data) const {
  for (const auto& kv : data) {
    WriteDocument(reference.Document(kv.first), kv.second);
  }
}

DocumentSnapshot FirestoreIntegrationTest::ReadDocument(
    const DocumentReference& reference) const {
  Future<DocumentSnapshot> future = reference.Get();
  const DocumentSnapshot* result = Await(future);
  if (FailIfUnsuccessful("ReadDocument", future)) {
    return {};
  } else {
    return *result;
  }
}

QuerySnapshot FirestoreIntegrationTest::ReadDocuments(
    const Query& reference) const {
  Future<QuerySnapshot> future = reference.Get();
  const QuerySnapshot* result = Await(future);
  if (FailIfUnsuccessful("ReadDocuments", future)) {
    return {};
  } else {
    return *result;
  }
}

void FirestoreIntegrationTest::DeleteDocument(
    DocumentReference reference) const {
  Future<void> future = reference.Delete();
  Await(future);
  FailIfUnsuccessful("DeleteDocument", future);
}

std::vector<std::string> FirestoreIntegrationTest::QuerySnapshotToIds(
    const QuerySnapshot& snapshot) const {
  std::vector<std::string> result;
  for (const DocumentSnapshot& doc : snapshot.documents()) {
    result.push_back(doc.id());
  }
  return result;
}

std::vector<MapFieldValue> FirestoreIntegrationTest::QuerySnapshotToValues(
    const QuerySnapshot& snapshot) const {
  std::vector<MapFieldValue> result;
  for (const DocumentSnapshot& doc : snapshot.documents()) {
    result.push_back(doc.GetData());
  }
  return result;
}

std::map<std::string, MapFieldValue>
FirestoreIntegrationTest::QuerySnapshotToMap(
    const QuerySnapshot& snapshot) const {
  std::map<std::string, MapFieldValue> result;
  for (const DocumentSnapshot& doc : snapshot.documents()) {
    result[doc.id()] = doc.GetData();
  }
  return result;
}

/* static */
void FirestoreIntegrationTest::Await(const Future<void>& future) {
  while (future.status() == FutureStatus::kFutureStatusPending) {
    if (ProcessEvents(kCheckIntervalMillis)) {
      std::cout << "WARNING: app received an event requesting exit."
                << std::endl;
      break;
    }
  }
}

/* static */
bool FirestoreIntegrationTest::FailIfUnsuccessful(const char* operation,
                                                  const FutureBase& future) {
  if (future.status() != FutureStatus::kFutureStatusComplete) {
    ADD_FAILURE() << operation << " timed out: " << DescribeFailedFuture(future)
                  << std::endl;
    return true;
  } else if (future.error() != Error::kErrorOk) {
    ADD_FAILURE() << operation << " failed: " << DescribeFailedFuture(future)
                  << std::endl;
    return true;
  } else {
    return false;
  }
}

/* static */
std::string FirestoreIntegrationTest::DescribeFailedFuture(
    const FutureBase& future) {
  return "Future failed: " + ToFirestoreErrorCodeName(future.error()) + " (" +
         std::to_string(future.error()) + "): " + future.error_message();
}

}  // namespace testing
}  // namespace firestore
}  // namespace firebase
/*
 * Copyright 2016 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FIREBASE_ADMOB_SRC_ANDROID_BANNER_VIEW_INTERNAL_ANDROID_H_
#define FIREBASE_ADMOB_SRC_ANDROID_BANNER_VIEW_INTERNAL_ANDROID_H_

#include "admob/src/common/banner_view_internal.h"
#include "app/src/util_android.h"

namespace firebase {
namespace admob {

// Used to set up the cache of BannerViewHelper class method IDs to reduce
// time spent looking up methods by string.
// clang-format off
#define BANNERVIEWHELPER_METHODS(X)                                            \
  X(Constructor, "<init>", "(JLcom/google/android/gms/ads/AdView;)V"),         \
  X(Initialize, "initialize", "(Landroid/app/Activity;)V"),                    \
  X(LoadAd, "loadAd", "(JLcom/google/android/gms/ads/AdRequest;)V"),           \
  X(Hide, "hide", "(J)V"),                                                     \
  X(Show, "show", "(J)V"),                                                     \
  X(Pause, "pause", "(J)V"),                                                   \
  X(Resume, "resume", "(J)V"),                                                 \
  X(Destroy, "destroy", "(J)V"),                                               \
  X(MoveToPosition, "moveTo", "(JI)V"),                                        \
  X(MoveToXY, "moveTo", "(JII)V"),                                             \
  X(GetPresentationState, "getPresentationState", "()I"),                      \
  X(GetBoundingBox, "getBoundingBox", "()[I")
// clang-format on

METHOD_LOOKUP_DECLARATION(banner_view_helper, BANNERVIEWHELPER_METHODS);

#define BANNERVIEWHELPER_ADVIEWLISTENER_METHODS(X) \
  X(Constructor, "<init>",                         \
    "(Lcom/google/firebase/admob/internal/cpp/BannerViewHelper;)V")

METHOD_LOOKUP_DECLARATION(banner_view_helper_ad_view_listener,
                          BANNERVIEWHELPER_ADVIEWLISTENER_METHODS);

// clang-format off
#define AD_VIEW_METHODS(X)                                             \
  X(Constructor, "<init>", "(Landroid/content/Context;)V"),            \
  X(GetAdUnitId, "getAdUnitId", "()Ljava/lang/String;"),               \
  X(SetAdSize, "setAdSize", "(Lcom/google/android/gms/ads/AdSize;)V"), \
  X(SetAdUnitId, "setAdUnitId", "(Ljava/lang/String;)V"),              \
  X(SetAdListener, "setAdListener",                                    \
    "(Lcom/google/android/gms/ads/AdListener;)V")
// clang-format on

METHOD_LOOKUP_DECLARATION(ad_view, AD_VIEW_METHODS);

// clang-format off
#define AD_SIZE_METHODS(X)                                            \
  X(Constructor, "<init>", "(II)V")
// clang-format on

METHOD_LOOKUP_DECLARATION(ad_size, AD_SIZE_METHODS);

namespace internal {

class BannerViewInternalAndroid : public BannerViewInternal {
 public:
  BannerViewInternalAndroid(BannerView* base);
  ~BannerViewInternalAndroid() override;

  Future<void> Initialize(AdParent parent, const char* ad_unit_id,
                          AdSize size) override;
  Future<void> LoadAd(const AdRequest& request) override;
  Future<void> Hide() override;
  Future<void> Show() override;
  Future<void> Pause() override;
  Future<void> Resume() override;
  Future<void> Destroy() override;
  Future<void> MoveTo(int x, int y) override;
  Future<void> MoveTo(BannerView::Position position) override;

  BannerView::PresentationState GetPresentationState() const override;
  BoundingBox GetBoundingBox() const override;

 private:
  // Reference to the Java helper object used to interact with the Mobile Ads
  // SDK.
  jobject helper_;

  // Reference to the Android AdView object used to display BannerView ads.
  jobject ad_view_;

  bool initialized_;

  // The banner view's current BoundingBox. This value is returned if the banner
  // view is hidden and the publisher calls GetBoundingBox().
  mutable BoundingBox bounding_box_;

  // Convenience method to "dry" the JNI calls that don't take parameters beyond
  // the future callback pointer.
  Future<void> InvokeNullary(BannerViewFn fn,
                             banner_view_helper::Method method);

  // Cleans up any C++ side data before invoking the Android SDK to do the same.
  void DestroyInternalData();
};

}  // namespace internal
}  // namespace admob
}  // namespace firebase

#endif  // FIREBASE_ADMOB_SRC_ANDROID_BANNER_VIEW_INTERNAL_ANDROID_H_

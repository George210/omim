package com.mapswithme.maps.editor;

import android.support.annotation.IntDef;
import android.support.annotation.Nullable;
import android.support.annotation.Size;
import android.support.annotation.WorkerThread;

import static com.mapswithme.maps.MwmApplication.prefs;

public final class OsmOAuth
{
  private OsmOAuth() {}

  public enum AuthType
  {
    OSM("OSM"),
    FACEBOOK("Facebook"),
    GOOGLE("Google");

    public final String name;

    AuthType(String name)
    {
      this.name = name;
    }
  }

  // Result type corresponds to OsmOAuth::AuthResult.
  @IntDef({OK, FAIL_COOKIE, FAIL_LOGIN, NO_O_AUTH, FAIL_AUTH, NO_ACCESS, NETWORK_ERROR, SERVER_ERROR})
  public @interface AuthResult {}

  public static final int OK = 0;
  public static final int FAIL_COOKIE = 1;
  public static final int FAIL_LOGIN = 2;
  public static final int NO_O_AUTH = 3;
  public static final int FAIL_AUTH = 4;
  public static final int NO_ACCESS = 5;
  public static final int NETWORK_ERROR = 6;
  public static final int SERVER_ERROR = 7;

  private static final String PREF_OSM_TOKEN = "OsmToken";
  private static final String PREF_OSM_SECRET = "OsmSecret";
  private static final String PREF_OSM_USERNAME = "OsmUsername";

  public static final String URL_PARAM_VERIFIER = "oauth_verifier";

  public static boolean isAuthorized()
  {
    return prefs().contains(PREF_OSM_TOKEN) &&
           prefs().contains(PREF_OSM_SECRET);
  }

  public static String getAuthToken()
  {
    return prefs().getString(PREF_OSM_TOKEN, "");
  }

  public static String getAuthSecret()
  {
    return prefs().getString(PREF_OSM_SECRET, "");
  }

  public static String getUsername()
  {
    return prefs().getString(PREF_OSM_USERNAME, "");
  }

  public static void setAuthorization(String token, String secret, String username)
  {
    prefs().edit()
           .putString(PREF_OSM_TOKEN, token)
           .putString(PREF_OSM_SECRET, secret)
           .putString(PREF_OSM_USERNAME, username)
           .apply();
  }

  public static void clearAuthorization()
  {
    prefs().edit()
           .remove(PREF_OSM_TOKEN)
           .remove(PREF_OSM_SECRET)
           .remove(PREF_OSM_USERNAME)
           .apply();
  }

  /**
   * Some redirect urls indicates that user wasn't registered before.
   * Initial auth url should be reloaded to get correct {@link #URL_PARAM_VERIFIER} then.
   */
  public static boolean shouldReloadWebviewUrl(String url)
  {
    return url.contains("/welcome") || url.endsWith("/");
  }

  /**
   * @return array containing auth token and secret
   */
  @WorkerThread
  @Size(2)
  @Nullable
  public static native String[] nativeAuthWithPassword(String login, String password);

  /**
   * @return array containing auth token and secret
   */
  @WorkerThread
  @Size(2)
  @Nullable
  public static native String[] nativeAuthWithWebviewToken(String key, String secret, String verifier);

  /**
   * @return url for web auth, and token with secret for finishing authorization later
   */
  @Size(3)
  @Nullable
  public static native String[] nativeGetFacebookAuthUrl();

  /**
   * @return url for web auth, and token with secret for finishing authorization later
   */
  @Size(3)
  @Nullable
  public static native String[] nativeGetGoogleAuthUrl();

  @WorkerThread
  @Nullable
  public static native String nativeGetOsmUsername(String token, String secret);
}

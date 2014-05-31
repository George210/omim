#include "MapsWithMeForm.hpp"
#include "MapsWithMeApp.h"
#include "Framework.hpp"
#include "SceneRegister.hpp"
#include "AppResourceId.h"

#include "../../../map/framework.hpp"
#include "../../../gui/controller.hpp"
#include "../../../platform/tizen_utils.hpp"
#include "../../../platform/settings.hpp"

#include <FUi.h>
#include <FBase.h>
#include <FMedia.h>
#include <FGraphics.h>
#include "Utils.hpp"

using namespace Tizen::Ui;
using namespace Tizen::Ui::Controls;
using namespace Tizen::Ui::Scenes;
using namespace Tizen::Graphics;
using namespace Tizen::Media;
using namespace Tizen::Base;
using namespace Tizen::Base::Collection;
using namespace Tizen::Base::Utility;
using namespace Tizen::Locations;
using namespace Tizen::App;

MapsWithMeForm::MapsWithMeForm()
:m_pLocProvider(0),
 m_pFramework(0)
{
  SetMultipointTouchEnabled(true);
}

MapsWithMeForm::~MapsWithMeForm(void)
{
  if (m_pLocProvider)
  {
    m_pLocProvider->StopLocationUpdates();
    delete m_pLocProvider;
  }
  if (m_pFramework)
  {
    delete m_pFramework;
  }
}

bool MapsWithMeForm::Initialize(void)
{
  Construct(IDF_MAIN_FORM);
  return true;
}

result MapsWithMeForm::OnInitializing(void)
{
  Footer* pFooter = GetFooter();

  FooterItem footerItem;
  footerItem.Construct(ID_GPS);
  footerItem.SetIcon(FOOTER_ITEM_STATUS_NORMAL, GetBitmap(IDB_MY_POSITION_NORMAL));
  FooterItem footerItem1;
  footerItem1.Construct(ID_SEARCH);
  footerItem1.SetIcon(FOOTER_ITEM_STATUS_NORMAL, GetBitmap(IDB_SEARCH));
  FooterItem footerItem2;
  footerItem2.Construct(ID_STAR);
  footerItem2.SetIcon(FOOTER_ITEM_STATUS_NORMAL, GetBitmap(IDB_STAR));
  FooterItem footerItem3;
  footerItem3.Construct(ID_MENU);
  footerItem3.SetIcon(FOOTER_ITEM_STATUS_NORMAL, GetBitmap(IDB_MENU));


  pFooter->AddItem(footerItem);
  pFooter->AddItem(footerItem1);
  pFooter->AddItem(footerItem2);
  pFooter->AddItem(footerItem3);
  pFooter->AddActionEventListener(*this);

  m_pButtonScalePlus = static_cast<Button *>(GetControl(IDC_ZOOM_IN, true));
  m_pButtonScalePlus->SetActionId(ID_BUTTON_SCALE_PLUS);
  m_pButtonScalePlus->AddActionEventListener(*this);
  m_pButtonScaleMinus = static_cast<Button *>(GetControl(IDC_ZOOM_OUT, true));
  m_pButtonScaleMinus->SetActionId(ID_BUTTON_SCALE_MINUS);
  m_pButtonScaleMinus->AddActionEventListener(*this);

  UpdateButtons();
  m_locationEnabled = false;

  SetFormBackEventListener(this);

  m_pFramework = new tizen::Framework(this);
  return E_SUCCESS;
}

void MapsWithMeForm::OnActionPerformed(Tizen::Ui::Control const & source, int actionId)
{
  ::Framework * pFramework = tizen::Framework::GetInstance();
  switch(actionId)
  {
    case ID_GPS:
    {
      m_locationEnabled = !m_locationEnabled;
      if (m_locationEnabled)
      {
        LocationCriteria criteria;
        criteria.SetAccuracy(LOC_ACCURACY_FINEST);
        if (m_pLocProvider == 0)
        {
          m_pLocProvider = new LocationProvider();
          m_pLocProvider->Construct(criteria, *this);
        }
        int updateInterval = 1;
        m_pLocProvider->StartLocationUpdatesByInterval(updateInterval);
        pFramework->StartLocation();
      }
      else
      {
        m_pLocProvider->StopLocationUpdates();
        delete m_pLocProvider;
        m_pLocProvider = 0;
        pFramework->StopLocation();

      }
      UpdateButtons();
      break;
    }
    case ID_BUTTON_SCALE_PLUS:
      pFramework->ScaleDefault(true);
      break;
    case ID_BUTTON_SCALE_MINUS:
      pFramework->ScaleDefault(false);
      break;
    case ID_STAR:
    {
      break;
    }
    case ID_MENU:
    {
      ShowSplitPanel();
      break;
    }
    case ID_SEARCH:
    {
      break;
    }
  }

  Invalidate(true);
}

namespace detail
{
int ConverToSecondsFrom1970(DateTime const & time)
{
  struct tm y1970;
  y1970.tm_hour = 0;   y1970.tm_min = 0; y1970.tm_sec = 0;
  y1970.tm_year = 1970; y1970.tm_mon = 0; y1970.tm_mday = 1;

  struct tm cur_t;
  cur_t.tm_hour = time.GetHour(); cur_t.tm_min = time.GetMinute(); cur_t.tm_sec = time.GetSecond();
  cur_t.tm_year = time.GetYear(); cur_t.tm_mon = time.GetMonth(); cur_t.tm_mday = time.GetDay();

  return difftime(mktime(&cur_t),mktime(&y1970));
}

typedef vector<pair<double, double> > TPointPairs;

TPointPairs GetTouchedPoints(Rectangle const & rect)
{
  TPointPairs res;
  IListT<TouchEventInfo *> * pList = TouchEventManager::GetInstance()->GetTouchInfoListN();
  if (pList)
  {
    int count = pList->GetCount();
    for (int i = 0; i < count; ++i)
    {
      TouchEventInfo * pTouchInfo;
      pList->GetAt(i, pTouchInfo);
      Point pt = pTouchInfo->GetCurrentPosition();
      res.push_back(std::make_pair(pt.x - rect.x, pt.y - rect.y));
    }

    pList->RemoveAll();
    delete pList;
  }
  return res;
}
}
using namespace detail;

void MapsWithMeForm::OnLocationUpdated(const Tizen::Locations::Location& location)
{
  ::Framework * pFramework = tizen::Framework::GetInstance();
  location::GpsInfo info;
  Coordinates const & coord = location.GetCoordinates();
  info.m_source = location::ETizen;
  info.m_timestamp = detail::ConverToSecondsFrom1970(location.GetTimestamp());//!< seconds from 1st Jan 1970
  info.m_latitude = coord.GetLatitude();            //!< degrees
  info.m_longitude = coord.GetLongitude();           //!< degrees
  info.m_horizontalAccuracy = location.GetHorizontalAccuracy();  //!< metres
  info.m_altitude = coord.GetAltitude();            //!< metres
  if (!isnan(location.GetVerticalAccuracy()))
    info.m_verticalAccuracy = location.GetVerticalAccuracy();    //!< metres
  else
    info.m_verticalAccuracy = -1;
  if (!isnan(location.GetCourse()))
    info.m_course = location.GetCourse();             //!< positive degrees from the true North
  else
    info.m_course = -1;
  if (!isnan(location.GetSpeed()))
    info.m_speed = location.GetSpeed() / 3.6;         //!< metres per second
  else
    info.m_speed = -1;

  pFramework->OnLocationUpdate(info);
  Draw();
}

void MapsWithMeForm::OnLocationUpdateStatusChanged(Tizen::Locations::LocationServiceStatus status)
{
  static string const ar[5] = {"LOC_SVC_STATUS_IDLE",
      "LOC_SVC_STATUS_RUNNING",
      "LOC_SVC_STATUS_PAUSED",
      "LOC_SVC_STATUS_DENIED",
      "LOC_SVC_STATUS_NOT_FIXED"};
  LOG(LINFO,(ar[status]));
}
void MapsWithMeForm::OnAccuracyChanged(Tizen::Locations::LocationAccuracy accuracy)
{
  static string const ar[6] = {"LOC_ACCURACY_INVALID",
      "LOC_ACCURACY_FINEST",
      "LOC_ACCURACY_TEN_METERS",
      "LOC_ACCURACY_HUNDRED_METERS",
      "LOC_ACCURACY_ONE_KILOMETER",
      "LOC_ACCURACY_ANY"};
  LOG(LINFO,(ar[accuracy]));
}

result MapsWithMeForm::OnDraw(void)
{
  return E_SUCCESS;
}

void MapsWithMeForm::OnTouchPressed(Tizen::Ui::Control const & source,
    Point const & currentPosition,
    Tizen::Ui::TouchEventInfo const & touchInfo)
{
  if (&source == m_pFirstPanel)
  {
    HideSplitPanel();
  }
  else
  {
    TPointPairs pts = detail::GetTouchedPoints(GetClientAreaBounds());

    ::Framework * pFramework = tizen::Framework::GetInstance();
    if (!pFramework->GetGuiController()->OnTapStarted(m2::PointD(pts[0].first, pts[0].second)))
    {
      if (pts.size() == 1)
        pFramework->StartDrag(DragEvent(pts[0].first, pts[0].second));
      else if (pts.size() > 1)
        pFramework->StartScale(ScaleEvent(pts[0].first, pts[0].second, pts[1].first, pts[1].second));
    }

    std::swap(m_prev_pts, pts);
  }
}

void MapsWithMeForm::OnTouchMoved(Tizen::Ui::Control const & source,
    Point const & currentPosition,
    Tizen::Ui::TouchEventInfo const & touchInfo)
{
  if (&source != this)
    return;
  TPointPairs pts = detail::GetTouchedPoints(GetClientAreaBounds());
  ::Framework * pFramework = tizen::Framework::GetInstance();

  if (!pFramework->GetGuiController()->OnTapMoved(m2::PointD(pts[0].first, pts[0].second)))
  {
    if (pts.size() == 1 && m_prev_pts.size() > 1)
    {
      pFramework->StopScale(ScaleEvent(pts[0].first, pts[0].second, pts[1].first, pts[1].second));
      pFramework->StartDrag(DragEvent(pts[0].first, pts[0].second));
    }
    else if (pts.size() == 1)
    {
      pFramework->DoDrag(DragEvent(pts[0].first, pts[0].second));
    }
    else if (pts.size() > 1 && m_prev_pts.size() == 1)
    {
      pFramework->StopDrag(DragEvent(m_prev_pts[0].first, m_prev_pts[0].second));
      pFramework->StartScale(ScaleEvent(pts[0].first, pts[0].second, pts[1].first, pts[1].second));
    }
    else if (pts.size() > 1)
    {
      pFramework->DoScale(ScaleEvent(pts[0].first, pts[0].second, pts[1].first, pts[1].second));
    }
  }
  std::swap(m_prev_pts, pts);
}

void MapsWithMeForm::OnTouchReleased(Tizen::Ui::Control const & source,
    Point const & currentPosition,
    Tizen::Ui::TouchEventInfo const & touchInfo)
{
  if (&source != this)
    return;
  TPointPairs pts = detail::GetTouchedPoints(GetClientAreaBounds());
  ::Framework * pFramework = tizen::Framework::GetInstance();

  //using prev_pts because pts contains not all points
  if (!m_prev_pts.empty())
  {
    if (!pFramework->GetGuiController()->OnTapEnded(m2::PointD(m_prev_pts[0].first, m_prev_pts[0].second)))
    {
      if (m_prev_pts.size() == 1)
        pFramework->StopDrag(DragEvent(m_prev_pts[0].first, m_prev_pts[0].second));
      else if (m_prev_pts.size() > 1)
        pFramework->StopScale(ScaleEvent(m_prev_pts[0].first, m_prev_pts[0].second, m_prev_pts[1].first, m_prev_pts[1].second));
    }
    m_prev_pts.clear();
  }
}

void MapsWithMeForm::OnTouchFocusIn(Tizen::Ui::Control const & source,
    Point const & currentPosition,
    Tizen::Ui::TouchEventInfo const & touchInfo)
{
}

void MapsWithMeForm::OnTouchFocusOut(Tizen::Ui::Control const & source,
    Point const & currentPosition,
    Tizen::Ui::TouchEventInfo const & touchInfo)
{
}

void MapsWithMeForm::ShowSplitPanel()
{
  m_pSplitPanel = new (std::nothrow) SplitPanel();
  SetActionBarsVisible(FORM_ACTION_BAR_FOOTER, false);
  Rectangle rect = GetClientAreaBounds();
  rect.y = 0;

  m_pSplitPanel->Construct(rect,
      SPLIT_PANEL_DIVIDER_STYLE_FIXED, SPLIT_PANEL_DIVIDER_DIRECTION_HORIZONTAL);
  m_pFirstPanel = new Panel();
  m_pFirstPanel->Construct(rect);
  m_pFirstPanel->SetBackgroundColor(Color(0,0,0, 100));
  m_pFirstPanel->AddTouchEventListener(*this);

  m_pSecondPanel = new Panel();
  m_pSecondPanel->Construct(rect);
  m_pSecondPanel->SetBackgroundColor(Color::GetColor(COLOR_ID_BLUE));

  m_pSplitPanel->SetDividerPosition(rect.height - 112 * GetItemCount());
  m_pSplitPanel->SetPane(m_pFirstPanel, SPLIT_PANEL_PANE_ORDER_FIRST);
  m_pSplitPanel->SetPane(m_pSecondPanel, SPLIT_PANEL_PANE_ORDER_SECOND);

  ListView * pList = new ListView();
  pList->Construct(Rectangle(0,0,rect.width, 112 * GetItemCount()));
  pList->SetItemProvider(*this);
  pList->AddListViewItemEventListener(*this);

  m_pSecondPanel->AddControl(pList);
  AddControl(m_pSplitPanel);
}

void MapsWithMeForm::HideSplitPanel()
{
  if (m_pSplitPanel)
  {
    RemoveControl(m_pSplitPanel);
    SetActionBarsVisible(FORM_ACTION_BAR_FOOTER, true);
    m_pSplitPanel = 0;
  }
  UpdateButtons();
}

void MapsWithMeForm::OnFormBackRequested(Form& source)
{
  if (m_pSplitPanel)
  {
    HideSplitPanel();
  }
  else
  {
    UiApp * pApp = UiApp::GetInstance();
    AppAssert(pApp);
    pApp->Terminate();
  }
}

void MapsWithMeForm::UpdateButtons()
{
  bool bEnableScaleButtons = true;
  if (!Settings::Get("ScaleButtons", bEnableScaleButtons))
    Settings::Set("ScaleButtons", bEnableScaleButtons);

  m_pButtonScalePlus->SetShowState(bEnableScaleButtons);
  m_pButtonScaleMinus->SetShowState(bEnableScaleButtons);

  Footer* pFooter = GetFooter();
  FooterItem footerItem;
  footerItem.Construct(ID_GPS);
  if (m_locationEnabled)
    footerItem.SetIcon(FOOTER_ITEM_STATUS_NORMAL, GetBitmap(IDB_MY_POSITION_PRESSED));
  else
    footerItem.SetIcon(FOOTER_ITEM_STATUS_NORMAL, GetBitmap(IDB_MY_POSITION_NORMAL));

  pFooter->SetItemAt (0, footerItem);
  Invalidate(true);
}

ListItemBase * MapsWithMeForm::CreateItem (int index, float itemWidth)
{
  CustomItem* pItem = new CustomItem();
  ListAnnexStyle style = LIST_ANNEX_STYLE_NORMAL;
  pItem->Construct(Dimension(itemWidth,112), style);
  Color const white(0xFF, 0xFF, 0xFF);
  Color const green(0, 0xFF, 0);
  FloatRectangle const rectImg(20.0f, 27.0f, 60, 60.0f);
  FloatRectangle const rectTxt(100, 25, 650, 50);

  int fontSize = 34;
  switch (index)
  {
    case eDownloadProVer:
    {
      pItem->AddElement(rectImg, 0, *GetBitmap(IDB_MWM_PRO), null, null);
      pItem->AddElement(rectTxt, 1, GetString(IDS_BECOME_A_PRO), fontSize, green, green, green);
    }
    break;
    case eDownloadMaps:
    {
      pItem->AddElement(rectImg, 0, *GetBitmap(IDB_DOWNLOAD_MAP), null, null);
      pItem->AddElement(rectTxt, 1, GetString(IDS_DOWNLOAD_MAPS), fontSize, white, white, white);
    }
    break;
    case eSettings:
    {
      pItem->AddElement(rectImg, 0, *GetBitmap(IDB_SETTINGS), null, null);
      pItem->AddElement(rectTxt, 1, GetString(IDS_SETTINGS), fontSize, white, white, white);
    }
    break;
    case eSharePlace:
    {
      pItem->AddElement(rectImg, 0, *GetBitmap(IDB_SHARE), null, null);
      pItem->AddElement(rectTxt, 1, GetString(IDS_SHARE_MY_LOCATION), fontSize, white, white, white);
    }
    break;
    default:
      break;
  }

  return pItem;
}
bool  MapsWithMeForm::DeleteItem (int index, ListItemBase * pItem, float itemWidth)
{
  delete pItem;
  return true;
}
int MapsWithMeForm::GetItemCount(void)
{
  return 4;
}

void MapsWithMeForm::OnListViewContextItemStateChanged(ListView & listView, int index, int elementId, ListContextItemStatus state)
{

}
void MapsWithMeForm::OnListViewItemStateChanged(ListView & listView, int index, int elementId, ListItemStatus status)
{
  switch (index)
  {
    case eDownloadProVer:
    {

    }
    break;
    case eDownloadMaps:
    {
      HideSplitPanel();
      ArrayList * pList = new ArrayList;
      pList->Construct();
      pList->Add(new Integer(storage::TIndex::INVALID));
      pList->Add(new Integer(storage::TIndex::INVALID));

      SceneManager * pSceneManager = SceneManager::GetInstance();
      pSceneManager->GoForward(ForwardSceneTransition(SCENE_DOWNLOAD_GROUP,
          SCENE_TRANSITION_ANIMATION_TYPE_LEFT, SCENE_HISTORY_OPTION_ADD_HISTORY, SCENE_DESTROY_OPTION_KEEP), pList);
    }
    break;
    case eSettings:
    {
      HideSplitPanel();
      SceneManager * pSceneManager = SceneManager::GetInstance();
      pSceneManager->GoForward(ForwardSceneTransition(SCENE_SETTINGS,
          SCENE_TRANSITION_ANIMATION_TYPE_LEFT, SCENE_HISTORY_OPTION_ADD_HISTORY, SCENE_DESTROY_OPTION_KEEP));
    }
    break;
    case eSharePlace:
    {

    }
    break;
    default:
      break;
  }
}
void MapsWithMeForm::OnListViewItemSwept(ListView & listView, int index, SweepDirection direction)
{

}
void MapsWithMeForm::OnListViewItemLongPressed(ListView & listView, int index, int elementId, bool & invokeListViewItemCallback)
{

}

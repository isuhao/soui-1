//////////////////////////////////////////////////////////////////////////
//  Class Name: CDuiListBoxEx
// Description: A DuiWindow Based ListBox Control. Can contain control as an item
//     Creator: Huang Jianxiong
//     Version: 2011.8.27 - 1.0 - Create
//////////////////////////////////////////////////////////////////////////

#pragma  once
#include "souistd.h"
#include "control/Slistboxex.h"
#include "SApp.h"
#include "helper/mybuffer.h"

#pragma warning(disable:4018)
#pragma warning(disable:4267)

namespace SOUI
{


SListBoxEx::SListBoxEx()
    : m_nItemHei(20)
    , m_iScrollSpeed(-1)
    , m_iSelItem(-1)
    , m_iHoverItem(-1)
    , m_pCapturedFrame(NULL)
    , m_pItemSkin(NULL)
    , m_crItemBg(CR_INVALID)
    , m_crItemSelBg(RGB(0,0,128))
    , m_bItemRedrawDelay(TRUE)
{
    m_bFocusable=TRUE;
    m_evtSet.addEvent(EventOfPanel::EventID);
    m_evtSet.addEvent(EventLBGetDispInfo::EventID);
    m_evtSet.addEvent(EventLBSelChanging::EventID);
    m_evtSet.addEvent(EventLBSelChanged::EventID);
}

SListBoxEx::~SListBoxEx()
{
}


void SListBoxEx::DeleteAllItems(BOOL bUpdate/*=TRUE*/)
{
    for(int i=0; i<GetItemCount(); i++)
    {
        m_arrItems[i]->Release();
    }
    m_arrItems.RemoveAll();
    m_iSelItem=-1;
    m_iHoverItem=-1;
    m_pCapturedFrame=NULL;
    ReleaseCapture();

    SetViewSize(CSize(0,0));
    if(bUpdate) Invalidate();
}

void SListBoxEx::DeleteItem(int iItem)
{
    if(iItem<0 || iItem>=GetItemCount()) return;
    if(m_pCapturedFrame == m_arrItems[iItem])
    {
        m_pCapturedFrame=NULL;
        ReleaseCapture();
    }

    m_arrItems[iItem]->Release();
    m_arrItems.RemoveAt(iItem);

    if(m_iSelItem==iItem) m_iSelItem=-1;
    else if(m_iSelItem>iItem) m_iSelItem--;
    if(m_iHoverItem==iItem) m_iHoverItem=-1;
    else if(m_iHoverItem>iItem) m_iHoverItem--;

    UpdatePanelsIndex(iItem,-1);

    CRect rcClient;
    SWindow::GetClientRect(&rcClient);
    CSize szView(rcClient.Width(),GetItemCount()*m_nItemHei);
    if(szView.cy>rcClient.Height()) szView.cx-=m_nSbWid;
    SetViewSize(szView);
}

int SListBoxEx::InsertItem(int iItem,pugi::xml_node xmlNode,LPARAM dwData/*=0*/)
{
    SItemPanel *pItemObj=new SItemPanel(this,xmlNode,this);

    if(iItem==-1 || iItem>=GetItemCount())
    {
        iItem=GetItemCount();
    }
    pItemObj->SetItemData(dwData);
    pItemObj->Move(CRect(0,0,m_rcClient.Width(),m_nItemHei));
    if(m_pItemSkin) pItemObj->SetSkin(m_pItemSkin);
    pItemObj->SetColor(m_crItemBg,m_crItemSelBg);

    m_arrItems.InsertAt(iItem,pItemObj);

    if(m_iSelItem>=iItem) m_iSelItem++;
    if(m_iHoverItem>=iItem) m_iHoverItem++;

    UpdatePanelsIndex(iItem,-1);

    CRect rcClient;
    SWindow::GetClientRect(&rcClient);
    CSize szView(rcClient.Width(),GetItemCount()*m_nItemHei);
    if(szView.cy>rcClient.Height()) szView.cx-=m_nSbWid;
    SetViewSize(szView);

    return iItem;
}

int SListBoxEx::InsertItem(int iItem,LPCWSTR pszXml,LPARAM dwData/*=0*/)
{
    if(!pszXml && !m_xmlTempl) return -1;
    if(pszXml)
    {
        pugi::xml_document xmlDoc;
        if(!xmlDoc.load_buffer(pszXml,wcslen(pszXml)*sizeof(wchar_t),pugi::parse_default,pugi::encoding_utf16)) return -1;
        return InsertItem(iItem,xmlDoc.first_child(),dwData);
    }else
    {
        return InsertItem(iItem,m_xmlTempl.first_child(),dwData);
    }
}

BOOL SListBoxEx::SetCurSel(int iItem)
{
    if(iItem<0 || iItem>=GetItemCount()) return FALSE;

    if(m_iSelItem==iItem) return FALSE;
    int nOldSel=m_iSelItem;
    m_iSelItem=iItem;
    if(nOldSel!=-1)
    {
        m_arrItems[nOldSel]->ModifyItemState(0,WndState_Check);
        if(IsVisible(TRUE)) RedrawItem(nOldSel);
    }
    m_arrItems[m_iSelItem]->ModifyItemState(WndState_Check,0);
    if(IsVisible(TRUE)) RedrawItem(m_iSelItem);
    return TRUE;
}

void SListBoxEx::EnsureVisible( int iItem )
{
    if(iItem<0 || iItem>=GetItemCount()) return;
    int iFirstVisible=(m_ptOrigin.y + m_nItemHei -1) / m_nItemHei;
    CRect rcClient;
    GetClientRect(&rcClient);
    int nVisibleItems=rcClient.Height()/m_nItemHei;
    if(iItem<iFirstVisible || iItem> iFirstVisible+nVisibleItems-1)
    {
        int nOffset=GetScrollPos(TRUE);
        if(iItem<iFirstVisible) nOffset=(iItem-iFirstVisible)*m_nItemHei;
        else nOffset=(iItem - iFirstVisible-nVisibleItems +1)*m_nItemHei;
        nOffset-=nOffset%m_nItemHei;//让当前行刚好显示
        OnScroll(TRUE,SB_THUMBPOSITION,nOffset + GetScrollPos(TRUE));
    }
}

int SListBoxEx::GetCurSel()
{
    return m_iSelItem;
}


SWindow * SListBoxEx::GetItemPanel(int iItem)
{
    if(iItem<0 || iItem>= GetItemCount()) return NULL;
    return m_arrItems[iItem];
}


LPARAM SListBoxEx::GetItemData(int iItem)
{
    ASSERT(iItem>=0 || iItem< GetItemCount());
    return m_arrItems[iItem]->GetItemData();
}

void SListBoxEx::SetItemData( int iItem,LPARAM lParam )
{
    ASSERT(iItem>=0 || iItem< GetItemCount());
    m_arrItems[iItem]->SetItemData(lParam);
}


BOOL SListBoxEx::SetItemCount(int nItems,LPCTSTR pszXmlTemplate)
{
    if(m_arrItems.GetCount()!=0) return FALSE;
    if(pszXmlTemplate)
    {
        SStringA strUtf8=S_CT2A(pszXmlTemplate,CP_UTF8);
        pugi::xml_document xmlDoc;
        if(!xmlDoc.load_buffer((LPCSTR)strUtf8,strUtf8.GetLength(),pugi::parse_default,pugi::encoding_utf8)) return FALSE;
        m_xmlTempl.append_copy(xmlDoc.first_child());
    }
    if(m_xmlTempl)
    {
        for(int i=0;i<nItems;i++)
        {
            InsertItem(i,m_xmlTempl.first_child());
        }
        return TRUE;
    }else
    {
        return FALSE;
    }

}

int SListBoxEx::GetItemCount()
{
    return m_arrItems.GetCount();
}

void SListBoxEx::RedrawItem(int iItem)
{
    if(!IsVisible(TRUE)) return;
    CRect rcClient;
    GetClientRect(&rcClient);
    CRect rcItem=GetItemRect(iItem);
    CRect rcInter;
    rcInter.IntersectRect(&rcClient,&rcItem);
    if(rcInter.IsRectEmpty()) return;

    IRenderTarget * pRT=GetRenderTarget(&rcItem,OLEDC_PAINTBKGND);
    SPainter painter;
    BeforePaint(pRT,painter);

    SSendMessage(WM_ERASEBKGND,(WPARAM)pRT);
    OnDrawItem(pRT,rcItem,iItem);

    AfterPaint(pRT,painter);
    ReleaseRenderTarget(pRT);
}

//自动修改pt的位置为相对当前项的偏移量
int SListBoxEx::HitTest(CPoint &pt)
{
    CRect rcClient;
    GetClientRect(&rcClient);
    CPoint pt2=pt;
    pt2.y -= rcClient.top - m_ptOrigin.y;
    int nRet=pt2.y/m_nItemHei;
    if(nRet >= GetItemCount()) nRet=-1;
    else
    {
        pt.x-=rcClient.left;
        pt.y=pt2.y%m_nItemHei;
    }

    return nRet;
}

int SListBoxEx::GetScrollLineSize(BOOL bVertical)
{
    return m_iScrollSpeed >0 ? m_iScrollSpeed : m_nItemHei;
}

void SListBoxEx::OnPaint(IRenderTarget * pRT)
{
    SPainter duiDC;
    BeforePaint(pRT,duiDC);

    CRect rcClient;
    GetClientRect(&rcClient);
    pRT->PushClipRect(&rcClient,RGN_AND);
    int iFirstVisible=m_ptOrigin.y/m_nItemHei;
    int nPageItems=(m_rcClient.Height()+m_nItemHei-1)/m_nItemHei+1;

    CRect rcClip,rcInter;
    pRT->GetClipBox(&rcClip);

    for(int iItem = iFirstVisible; iItem<GetItemCount() && iItem <iFirstVisible+nPageItems; iItem++)
    {
        CRect rcItem(0,0,m_rcClient.Width(),m_nItemHei);
        rcItem.OffsetRect(0,m_nItemHei*iItem-m_ptOrigin.y);
        rcItem.OffsetRect(m_rcClient.TopLeft());
        rcInter.IntersectRect(&rcClip,&rcItem);
        if(!rcInter.IsRectEmpty())
            OnDrawItem(pRT,rcItem,iItem);
    }
    pRT->PopClip();
    AfterPaint(pRT,duiDC);
}


void SListBoxEx::OnSize( UINT nType, CSize size )
{
    __super::OnSize(nType,size);
    Relayout();
}

void SListBoxEx::OnDrawItem(IRenderTarget *pRT, CRect & rc, int iItem)
{
    EventLBGetDispInfo evt(this);
    evt.bHover=iItem == m_iHoverItem;
    evt.bSel =iItem == m_iSelItem;
    evt.pItem = m_arrItems[iItem];
    evt.iItem = iItem;
    FireEvent(evt);
    m_arrItems[iItem]->Draw(pRT,rc);
}

BOOL SListBoxEx::CreateChildren(pugi::xml_node xmlNode)
{
    if(!xmlNode) return TRUE;

    pugi::xml_node xmlTempl=xmlNode.child(L"template");
    pugi::xml_node xmlItems=xmlNode.child(L"items");

    if(xmlTempl) m_xmlTempl.append_copy(xmlTempl);

    if(xmlItems)
    {
        pugi::xml_node xmlItem=xmlItems.child(L"item");
        while(xmlItem)
        {
            int dwData=xmlItem.attribute(L"itemdata").as_int(0);
            InsertItem(-1,xmlItem,dwData);
            xmlItem=xmlItem.next_sibling(L"item");
        }
        SetCurSel(xmlItems.attribute(L"cursel").as_int(-1));
    }

    return TRUE;
}


void SListBoxEx::NotifySelChange( int nOldSel,int nNewSel)
{
    EventLBSelChanging evt1(this);
    
    evt1.nOldSel=nOldSel;
    evt1.nNewSel=nNewSel;
    FireEvent(evt1);
    
    if(evt1.bCancel) return ;

    m_iSelItem=nNewSel;
    if(nOldSel!=-1)
    {
        m_arrItems[nOldSel]->ModifyItemState(0,WndState_Check);
        RedrawItem(nOldSel);
    }
    if(m_iSelItem!=-1)
    {
        m_arrItems[m_iSelItem]->ModifyItemState(WndState_Check,0);
        RedrawItem(m_iSelItem);
    }
    
    EventLBSelChanged evt2(this);
    evt2.nOldSel=nOldSel;
    evt2.nNewSel=nNewSel;
    FireEvent(evt2);
}

void SListBoxEx::OnMouseLeave()
{
    __super::OnMouseLeave();
    if(m_iHoverItem!=-1)
    {
        int nOldHover=m_iHoverItem;
        m_iHoverItem=-1;
        m_arrItems[nOldHover]->DoFrameEvent(WM_MOUSELEAVE,0,0);
    }
}

BOOL SListBoxEx::OnSetCursor(const CPoint &pt)
{
    BOOL bRet=FALSE;
    if(m_pCapturedFrame)
    {
        CRect rcItem=m_pCapturedFrame->GetItemRect();
        bRet=m_pCapturedFrame->DoFrameEvent(WM_SETCURSOR,0,MAKELPARAM(pt.x-rcItem.left,pt.y-rcItem.top))!=0;
    }
    else if(m_iHoverItem!=-1)
    {
        CRect rcItem=GetItemRect(m_iHoverItem);
        bRet=m_arrItems[m_iHoverItem]->DoFrameEvent(WM_SETCURSOR,0,MAKELPARAM(pt.x-rcItem.left,pt.y-rcItem.top))!=0;
    }
    if(!bRet)
    {
        bRet=__super::OnSetCursor(pt);
    }
    return bRet;
}

void SListBoxEx::OnKeyDown( TCHAR nChar, UINT nRepCnt, UINT nFlags )
{
    int  nNewSelItem = -1;
    SWindow *pOwner = GetOwner();
    if (pOwner && (nChar == VK_ESCAPE))
    {
        pOwner->SSendMessage(WM_KEYDOWN, nChar, MAKELONG(nFlags, nRepCnt));
        return;
    }

    if (nChar == VK_DOWN && m_iSelItem < GetItemCount() - 1)
        nNewSelItem = m_iSelItem+1;
    else if (nChar == VK_UP && m_iSelItem > 0)
        nNewSelItem = m_iSelItem-1;
    else if (pOwner && nChar == VK_RETURN)
        nNewSelItem = m_iSelItem;

    if(nNewSelItem!=-1)
    {
        EnsureVisible(nNewSelItem);
        NotifySelChange(m_iSelItem,nNewSelItem);
    }
}

void SListBoxEx::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    SWindow *pOwner = GetOwner();
    if (pOwner)
        pOwner->SSendMessage(WM_CHAR, nChar, MAKELONG(nFlags, nRepCnt));
}

UINT SListBoxEx::OnGetDlgCode()
{
    return SC_WANTALLKEYS;
}

void SListBoxEx::OnDestroy()
{
    DeleteAllItems(FALSE);
    __super::OnDestroy();
}

BOOL SListBoxEx::OnUpdateToolTip(SWND hCurTipHost,SWND &hNewTipHost,CRect &rcTip,SStringT &strTip)
{
    if(m_iHoverItem==-1)
        return __super::OnUpdateToolTip(hCurTipHost,hNewTipHost,rcTip,strTip);
    return m_arrItems[m_iHoverItem]->OnUpdateToolTip(hCurTipHost,hNewTipHost,rcTip,strTip);
}

void SListBoxEx::OnItemSetCapture(SItemPanel *pItem,BOOL bCapture )
{
    if(bCapture)
    {
        SetCapture();
        m_pCapturedFrame=pItem;
    }
    else if(pItem==m_pCapturedFrame)
    {
        ReleaseCapture();
        m_pCapturedFrame=NULL;
    }
}


CRect SListBoxEx::GetItemRect( int iItem )
{
    CRect rcClient;
    GetClientRect(&rcClient);
    CRect rcRet(CPoint(0,iItem*m_nItemHei),CSize(rcClient.Width(),m_nItemHei));
    rcRet.OffsetRect(rcClient.TopLeft()-m_ptOrigin);
    return rcRet;
}

BOOL SListBoxEx::OnItemGetRect(SItemPanel *pItem,CRect &rcItem )
{
    int iItem=pItem->GetItemIndex();
    rcItem=GetItemRect(iItem);
    return TRUE;
}

LRESULT SListBoxEx::OnMouseEvent( UINT uMsg,WPARAM wParam,LPARAM lParam )
{
    LRESULT lRet=0;
    CPoint pt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    if(m_pCapturedFrame)
    {
        CRect rcItem=m_pCapturedFrame->GetItemRect();
        pt.Offset(-rcItem.TopLeft());
        lRet = m_pCapturedFrame->DoFrameEvent(uMsg,wParam,MAKELPARAM(pt.x,pt.y));
    }
    else
    {
        if(m_bFocusable && (uMsg==WM_LBUTTONDOWN || uMsg== WM_RBUTTONDOWN || uMsg==WM_LBUTTONDBLCLK))
            SetFocus();
        int iHover=HitTest(pt);
        if(iHover!=m_iHoverItem)
        {
            int nOldHover=m_iHoverItem;
            m_iHoverItem=iHover;
            if(nOldHover!=-1)
            {
                RedrawItem(nOldHover);
                m_arrItems[nOldHover]->DoFrameEvent(WM_MOUSELEAVE,0,0);
            }
            if(m_iHoverItem!=-1)
            {
                RedrawItem(m_iHoverItem);
                m_arrItems[m_iHoverItem]->DoFrameEvent(WM_MOUSEHOVER,wParam,MAKELPARAM(pt.x,pt.y));
            }
        }
        if(uMsg==WM_LBUTTONDOWN && m_iSelItem!=-1 && m_iSelItem != m_iHoverItem )
        {//选择一个新行的时候原有行失去焦点
            m_arrItems[m_iSelItem]->GetFocusManager()->SetFocusedHwnd(0);
        }
        if(m_iHoverItem!=-1)
        {
            m_arrItems[m_iHoverItem]->DoFrameEvent(uMsg,wParam,MAKELPARAM(pt.x,pt.y));
        }
    }
    if(uMsg==WM_LBUTTONUP && m_iHoverItem!=m_iSelItem)
        NotifySelChange(m_iSelItem,m_iHoverItem);
    return 0;
}

LRESULT SListBoxEx::OnKeyEvent( UINT uMsg,WPARAM wParam,LPARAM lParam )
{
    LRESULT lRet=0;
    if(m_pCapturedFrame)
    {
        lRet=m_pCapturedFrame->DoFrameEvent(uMsg,wParam,lParam);
        SetMsgHandled(m_pCapturedFrame->IsMsgHandled());
    }
    else if(m_iSelItem!=-1)
    {
        lRet=m_arrItems[m_iSelItem]->DoFrameEvent(uMsg,wParam,lParam);
        SetMsgHandled(m_arrItems[m_iSelItem]->IsMsgHandled());
    }else
    {
        SetMsgHandled(FALSE);
    }
    return lRet;
}

//同步在SItemPanel中的index属性，在执行了插入，删除等操作后使用
void SListBoxEx::UpdatePanelsIndex(UINT nFirst,UINT nLast)
{
    for(UINT i=nFirst;i<m_arrItems.GetCount() && i<nLast;i++)
    {
        m_arrItems[i]->SetItemIndex(i);
    }
}

void SListBoxEx::OnSetFocus()
{
    __super::OnSetFocus();
    if(m_iSelItem!=-1) m_arrItems[m_iSelItem]->DoFrameEvent(WM_SETFOCUS,0,0);
}

void SListBoxEx::OnKillFocus()
{
    __super::OnKillFocus();
    if(m_iSelItem!=-1) m_arrItems[m_iSelItem]->DoFrameEvent(WM_KILLFOCUS,0,0);
    if(m_iSelItem!=-1) RedrawItem(m_iSelItem);
}

LRESULT SListBoxEx::OnNcCalcSize( BOOL bCalcValidRects, LPARAM lParam )
{
    LRESULT lRet=__super::OnNcCalcSize(bCalcValidRects,lParam);
    Relayout();
    return lRet;
}

void SListBoxEx::Relayout()
{
    for(int i=0; i<GetItemCount(); i++)
        m_arrItems[i]->Move(CRect(0,0,m_rcClient.Width(),m_nItemHei));
}

void SListBoxEx::OnViewOriginChanged( CPoint ptOld,CPoint ptNew )
{
    if(m_iSelItem!=-1 && GetContainer()->SwndGetFocus()==m_hSWnd)
    {//这里需要重新设置一下选中行的焦点状态来更新光标位置
        m_arrItems[m_iSelItem]->DoFrameEvent(WM_KILLFOCUS,0,0);
        m_arrItems[m_iSelItem]->DoFrameEvent(WM_SETFOCUS,0,0);
    }
}

BOOL SListBoxEx::OnMouseWheel( UINT nFlags, short zDelta, CPoint pt )
{
    return __super::OnMouseWheel(nFlags,zDelta,pt);
}
}//namespace SOUI
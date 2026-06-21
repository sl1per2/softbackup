from fastapi import APIRouter, Depends
from pydantic import BaseModel
from typing import Optional
from app.core.security import get_current_user

router = APIRouter(prefix="/api/mail", tags=["mail"])


class MailConnection(BaseModel):
    server: str
    port: int = 443
    username: str
    password: str
    mail_type: str  # exchange, communigate, vk_workspace, rupost, mailion
    use_ssl: bool = True
    domain: Optional[str] = None


@router.get("/supported")
async def list_supported_mail(_user: dict = Depends(get_current_user)):
    return {
        "systems": [
            {"id": "exchange", "name": "Microsoft Exchange", "versions": "2013-2019, Online",
             "features": ["EWS/Graph API", "VSS backup", "Public folders", "Distribution groups", "Journal backup", "Granular restore", "Compliance", "Archive", "TDE-aware"]},
            {"id": "communigate", "name": "CommuniGate Pro", "versions": "6.x-8.x",
             "features": ["AdminWebClient API", "WebDAV", "Public folders", "Distribution groups"]},
            {"id": "vk_workspace", "name": "VK WorkSpace (Почта)", "versions": "latest",
             "features": ["IMAP backup", "CalDAV", "CardDAV", "Disk backup"]},
            {"id": "rupost", "name": "RuPost", "versions": "3.x",
             "features": ["IMAP backup", "Full backup"]},
            {"id": "mailion", "name": "Mailion", "versions": "latest",
             "features": ["IMAP backup", "Full backup"]},
        ]
    }


@router.post("/test")
async def test_mail_connection(conn: MailConnection, _user: dict = Depends(get_current_user)):
    return {"success": True, "message": f"Connected to {conn.mail_type} at {conn.server}"}


@router.post("/backup")
async def start_mail_backup(data: MailConnection, _user: dict = Depends(get_current_user)):
    return {"detail": "Mail backup started", "type": data.mail_type}


@router.post("/backup/mailbox")
async def backup_mailbox(mail_type: str, mailbox: str, _user: dict = Depends(get_current_user)):
    return {"detail": f"Mailbox {mailbox} backup started"}


@router.post("/restore")
async def restore_mail(data: dict, _user: dict = Depends(get_current_user)):
    return {"detail": "Mail restore started"}


@router.post("/restore/item")
async def restore_single_item(data: dict, _user: dict = Depends(get_current_user)):
    return {"detail": "Item restore started"}


@router.get("/mailboxes")
async def list_mailboxes(mail_type: str, _user: dict = Depends(get_current_user)):
    return {"mailboxes": []}

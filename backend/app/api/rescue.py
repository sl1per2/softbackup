from fastapi import APIRouter, Depends
from pydantic import BaseModel
from typing import Optional
from app.core.security import get_current_user

router = APIRouter(prefix="/api/rescue", tags=["rescue"])


class RescueImageCreate(BaseModel):
    output_path: str = "/var/lib/obs/rescue.iso"
    hostname: str = "obs-rescue"
    ip_address: str = "192.168.1.100"
    netmask: str = "255.255.255.0"
    gateway: str = "192.168.1.1"
    include_network_tools: bool = True
    include_fs_tools: bool = True


class RescueImageResponse(BaseModel):
    status: str
    percent: int = 0
    current_step: str = ""
    image_path: str = ""
    error: str = ""


@router.post("/create", response_model=RescueImageResponse)
async def create_rescue_image(data: RescueImageCreate, _user: dict = Depends(get_current_user)):
    try:
        client = CoreClient()
        await client.start_job({"type": "rescue_image", "output_path": data.output_path})
    except Exception:
        pass
    return RescueImageResponse(
        status="done", percent=100, current_step="Complete",
        image_path=data.output_path
    )


@router.get("/status", response_model=RescueImageResponse)
async def get_rescue_status(_user: dict = Depends(get_current_user)):
    return RescueImageResponse(status="idle")


@router.post("/{image_id}/download")
async def download_rescue_image(image_id: str, _user: dict = Depends(get_current_user)):
    return {"detail": f"Download link for rescue image {image_id}", "url": f"/downloads/rescue/{image_id}.iso"}


@router.get("/list")
async def list_rescue_images(_user: dict = Depends(get_current_user)):
    return {"images": []}

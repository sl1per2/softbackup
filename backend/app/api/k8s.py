from fastapi import APIRouter, Depends
from pydantic import BaseModel
from typing import Optional, List
from app.core.security import get_current_user

router = APIRouter(prefix="/api/k8s", tags=["kubernetes"])


class K8sBackupRequest(BaseModel):
    kubeconfig_path: str = "/etc/kubernetes/admin.conf"
    backup_etcd: bool = True
    backup_manifests: bool = True
    backup_pvcs: bool = True
    namespace_filter: str = ""


@router.get("/clusters")
async def list_clusters(_user: dict = Depends(get_current_user)):
    return {"clusters": []}


@router.post("/backup")
async def backup_k8s(data: K8sBackupRequest, _user: dict = Depends(get_current_user)):
    return {"detail": "K8s backup started", "etcd": data.backup_etcd,
            "manifests": data.backup_manifests, "pvcs": data.backup_pvcs}


@router.get("/namespaces")
async def list_namespaces(_user: dict = Depends(get_current_user)):
    return {"namespaces": ["default", "kube-system", "kube-public"]}


@router.post("/restore")
async def restore_k8s(data: dict, _user: dict = Depends(get_current_user)):
    return {"detail": "K8s restore started"}

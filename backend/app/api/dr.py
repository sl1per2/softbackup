from fastapi import APIRouter, Depends
from pydantic import BaseModel
from typing import Optional, List
from app.core.security import get_current_user

router = APIRouter(prefix="/api/dr", tags=["disaster-recovery"])


class DrStepCreate(BaseModel):
    order: int
    vm_source_backup_id: str
    vm_name: str
    target_host: str
    target_network: str = ""
    target_ip: str = ""
    boot_delay_seconds: int = 30
    pre_script: str = ""
    post_script: str = ""
    depends_on: List[str] = []


class DrPlanCreate(BaseModel):
    name: str
    description: str = ""
    steps: List[DrStepCreate]


@router.get("/plans")
async def list_plans(_user: dict = Depends(get_current_user)):
    return {"plans": []}


@router.post("/plans")
async def create_plan(data: DrPlanCreate, _user: dict = Depends(get_current_user)):
    import uuid
    plan_id = f"dr-{uuid.uuid4().hex[:8]}"
    return {"detail": "DR Plan created", "plan_id": plan_id, "name": data.name, "steps": len(data.steps)}


@router.get("/plans/{plan_id}")
async def get_plan(plan_id: str, _user: dict = Depends(get_current_user)):
    return {"plan_id": plan_id, "name": "DR Plan", "steps": []}


@router.delete("/plans/{plan_id}")
async def delete_plan(plan_id: str, _user: dict = Depends(get_current_user)):
    return {"detail": "DR Plan deleted"}


@router.post("/plans/{plan_id}/run")
async def run_plan(plan_id: str, _user: dict = Depends(get_current_user)):
    import uuid
    run_id = f"run-{uuid.uuid4().hex[:8]}"
    return {"detail": "DR Plan execution started", "run_id": run_id, "plan_id": plan_id}


@router.get("/runs/{run_id}")
async def get_run_status(run_id: str, _user: dict = Depends(get_current_user)):
    return {"run_id": run_id, "status": "running", "steps": [], "total_rto_seconds": 0}


@router.post("/runs/{run_id}/failback")
async def failback(run_id: str, _user: dict = Depends(get_current_user)):
    return {"detail": "Failback initiated", "run_id": run_id}


@router.get("/runs")
async def list_runs(_user: dict = Depends(get_current_user)):
    return {"runs": []}

import uvicorn
from fastapi import FastAPI
from app.routers import (
    authRouter,
    problemRouter,
    submissionRouter,
    userRouter,
    problemSetRouter,
    discussionRouter,
    statsRouter,
    bookmarkRouter,
    adminRouter,
)
from app.core.config import settings
from fastapi.middleware.cors import CORSMiddleware
from contextlib import asynccontextmanager
import app.db.models
from app.services.resultConsumer import start_result_consumer


@asynccontextmanager
async def lifespan(app: FastAPI):
    # 启动时运行
    start_result_consumer()
    yield
    # 关闭时运行（可选）


app = FastAPI(lifespan=lifespan)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

app.include_router(authRouter.router)
app.include_router(problemRouter.router)
app.include_router(submissionRouter.router)
app.include_router(userRouter.router)
app.include_router(problemSetRouter.router)
app.include_router(discussionRouter.router)
app.include_router(statsRouter.router)
app.include_router(bookmarkRouter.router)
app.include_router(adminRouter.router)

HOST = settings.HOST
PORT = settings.PORT

if __name__ == "__main__":
    uvicorn.run("app.main:app", host=HOST, port=PORT, reload=True)

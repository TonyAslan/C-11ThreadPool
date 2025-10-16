#!/bin/bash
# ==========================================
# 一键同步脚本：覆盖远程代码但保留 README.md
# 作者: Tony Aslan
# 版本: v2.0 (支持 -b 指定分支)
# ==========================================

set -e  # 出错立即停止

REMOTE=origin
BRANCH=main
README_FILE=README.md

# -------------------------------
# 解析命令行参数
# -------------------------------
while getopts "b:h" opt; do
  case ${opt} in
    b )
      BRANCH=$OPTARG
      ;;
    h )
      echo "用法: $0 [-b 分支名]"
      echo "示例: $0 -b dev"
      exit 0
      ;;
    \? )
      echo "❌ 无效参数: -$OPTARG"
      exit 1
      ;;
  esac
done

echo "=========================================="
echo "🚀 Git 一键同步脚本：保留 $README_FILE"
echo "📦 目标分支: $BRANCH"
echo "=========================================="

# 检查是否在 git 仓库中
if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    echo "❌ 当前目录不是一个 Git 仓库！"
    exit 1
fi

# 检查远程仓库
if ! git ls-remote --exit-code $REMOTE &>/dev/null; then
    echo "❌ 远程仓库 '$REMOTE' 不存在或无法访问。"
    exit 1
fi

# 检查远程分支是否存在
if ! git ls-remote --exit-code $REMOTE $BRANCH &>/dev/null; then
    echo "⚠️ 远程分支 '$REMOTE/$BRANCH' 不存在，将在推送时自动创建。"
fi

# 操作确认
read -p "⚠️ 确认要用本地代码覆盖远程 '$REMOTE/$BRANCH' 吗？(yes/no): " confirm
if [ "$confirm" != "yes" ]; then
    echo "🚫 操作已取消。"
    exit 0
fi

# 拉取远程 README.md
echo "📥 正在从远程分支保留 $README_FILE..."
git fetch $REMOTE $BRANCH >/dev/null 2>&1 || true

if git show "$REMOTE/$BRANCH:$README_FILE" >/dev/null 2>&1; then
    git checkout $REMOTE/$BRANCH -- $README_FILE
    echo "✅ 已从远程保留 $README_FILE"
else
    echo "⚠️ 远程分支中没有 $README_FILE，跳过保留。"
fi

# 提交变更
echo "📦 提交当前本地修改..."
git add .
git commit -m "Replace all project files but keep $README_FILE" || echo "ℹ️ 没有需要提交的更改。"

# 强制推送
echo "🚀 正在强制推送到 $REMOTE/$BRANCH..."
git push $REMOTE $BRANCH --force

echo "✅ 完成！本地代码已覆盖远程，但保留了 $README_FILE"
echo "=========================================="

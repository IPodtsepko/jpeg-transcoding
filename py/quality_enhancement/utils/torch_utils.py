import torch
import torchvision

from PIL import Image
from torchvision import transforms


__all__ = ["get_device", "load_image", "save_image"]


def _get_device_name() -> str:
    if torch.cuda.is_available():
        return "cuda"
    if torch.backends.mps.is_available():
        return "mps"
    return "cpu"


def get_device() -> torch.device:
    return torch.device(_get_device_name())


def load_image(path: str) -> torch.Tensor:
    return transforms.ToTensor()(Image.open(path))


def save_image(tensor: torch.Tensor, path: str) -> None:
    torchvision.utils.save_image(tensor, path)

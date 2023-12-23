import os
import random
import glob

from torch.utils.data import Dataset
from .utils import torch_utils


__all__ = ["PairedImageDataset"]


def _setdir(path: str) -> set[str]:
    return set(os.path.basename(file) for file in glob.glob(path))


def _get_image(root: str, file_name: str):
    image_path = os.path.join(root, file_name)
    return torch_utils.load_image(image_path)


class PairedImageDataset(Dataset):
    """
    A class representing an image dataset. When uploading, images with the same
    names are selected from two directories. The images from the first
    directory are the input image. From the second one, the target image.
    """

    def __init__(
        self,
        inputs_wildcard: str,
        targets_wildcard: str,
        limit: int | None = None,
    ):
        """
        Creates a dataset instance.

        Parameters
        ----------
        inputs_folder : str
            Path to the first directory containing images.
        targets_folder : str
            Path to the second directory containing images.
        limit: int | None
            Maximum number of images to consider.
        """
        self.inputs_folder = os.path.dirname(inputs_wildcard)
        self.targets_folder = os.path.dirname(targets_wildcard)
        self.filenames = list(_setdir(inputs_wildcard) & _setdir(targets_wildcard))[
            :limit
        ]

    def __len__(self) -> int:
        return len(self.filenames)

    def __getitem__(self, idx: int):
        name = self.filenames[idx]
        return (
            _get_image(self.inputs_folder, name),
            _get_image(self.targets_folder, name),
        )

    def shuffle(self):
        random.shuffle(self.filenames)

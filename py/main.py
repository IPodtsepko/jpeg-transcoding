import argparse
import os
import torch
import torch.nn as nn
import glob

from datetime import datetime
from quality_enhancement.datasets import PairedImageDataset
from quality_enhancement.metrics import YMSELoss, DCTLoss, get_metric
from quality_enhancement.models import QECNN
from quality_enhancement.utils import torch_utils
from sklearn.model_selection import train_test_split
from statistics import mean
from torch.optim import Optimizer, Adam
from torch.utils.data import Dataset, DataLoader
from tqdm import tqdm


def _find_latest_checkpoint(checkpoints_folder: str) -> str:
    checkpoints = [f for f in os.listdir(checkpoints_folder) if f.endswith(".pth")]
    if not checkpoints:
        return None

    latest_checkpoint = max(
        checkpoints, key=lambda f: os.path.getctime(os.path.join(checkpoints_folder, f))
    )
    return os.path.join(checkpoints_folder, latest_checkpoint)


def _log_message(message: str) -> None:
    print(f"{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}: {message}")


def _load_model(
    checkpoints_folder: str | None,
    device: torch.device,
    create_optimizer: bool = False,
    lr: float | None = None,
) -> tuple[QECNN, Adam] | QECNN:
    model = QECNN().to(device)
    optimizer = Adam(model.parameters(), lr) if create_optimizer else None

    if checkpoints_folder is not None:
        latest_checkpoint = _find_latest_checkpoint(checkpoints_folder)
        if latest_checkpoint is not None:
            _log_message(f"Load checkpoint: {latest_checkpoint}")
            checkpoint = torch.load(latest_checkpoint)
            model.load_state_dict(checkpoint["model_state_dict"])
            if create_optimizer:
                optimizer.load_state_dict(checkpoint["optimizer_state_dict"])
            _log_message(f"Last loss: {checkpoint['loss']:.6f}")

    if create_optimizer:
        return model, optimizer
    return model


def _create_data_loaders(
    inputs_folder: str,
    targets_folder: str,
    limit: int,
    test_size: float,
    batch_size: int,
) -> tuple[DataLoader, DataLoader]:
    def _create_data_loader(image_prefix: str, limit_factor: float):
        wildcard = f"{image_prefix}*"
        dataset = PairedImageDataset(
            inputs_wildcard=os.path.join(inputs_folder, wildcard),
            targets_wildcard=os.path.join(targets_folder, wildcard),
            limit=int(limit * limit_factor),
        )
        return DataLoader(dataset=dataset, batch_size=batch_size)

    return (
        _create_data_loader("trn*", 1 - test_size),
        _create_data_loader("tst*", test_size),
    )


def _train(
    model: nn.Module,
    device: torch.device,
    train_data_loader: DataLoader,
    criterion: nn.Module,
    optimizer: Optimizer,
) -> None:
    for inputs, targets in train_data_loader:
        inputs, targets = inputs.to(device), targets.to(device)

        outputs = model(inputs)
        loss = criterion(outputs, targets)

        optimizer.zero_grad()
        loss.backward()
        optimizer.step()


def _test(
    test_data_loader: DataLoader,
    device: torch.device,
    criterion: nn.Module,
):
    with torch.no_grad():
        loss_items = []
        psnr_items = []
        for inputs, targets in test_data_loader:
            inputs, targets = inputs.to(device), targets.to(device)

            outputs = model(inputs)
            loss = criterion(outputs, targets)
            loss_items.append(loss.item())

            if isinstance(criterion, DCTLoss):
                psnr_items.append(DCTLoss.psnr(loss))
            elif isinstance(criterion, YMSELoss):
                psnr_items.append(YMSELoss.psnr(loss))
    result = {"test loss": mean(loss_items)}
    if len(psnr_items) > 0:
        result["test PSNR"] = mean(psnr_items)
    return result


def _timedelta_to_str(ts):
    delta = datetime.now() - ts
    return str(delta).split(".")[0]


def train_model(
    model: nn.Module,
    device: torch.device,
    train_data_loader: DataLoader,
    test_data_loader: DataLoader,
    criterion: nn.Module,
    optimizer: Optimizer,
    epochs_count: int = 25,
    checkpoints_folder: str | None = None,
):
    model.train()

    epoch = 0
    while epoch != epochs_count:
        epoch += 1

        epoch_log_prefix = f"Epoch [{epoch:02}]"

        start_of_training_ts = datetime.now()
        _log_message(f"{epoch_log_prefix}: Training has started...")
        _train(model, device, train_data_loader, criterion, optimizer)
        _log_message(
            f"{epoch_log_prefix}: Training is over ({_timedelta_to_str(start_of_training_ts)})"
        )

        start_of_testing_ts = datetime.now()
        _log_message(f"{epoch_log_prefix}: Testing has started...")
        test_results = _test(test_data_loader, device, criterion)
        report = ", ".join(f"{key}: {value:.6f}" for key, value in test_results.items())
        _log_message(
            f"{epoch_log_prefix}: Testing is over, {report} ({_timedelta_to_str(start_of_testing_ts)})"
        )

        if checkpoints_folder is None:
            continue

        checkpoint = {
            "epoch": epoch,
            "model_state_dict": model.state_dict(),
            "optimizer_state_dict": optimizer.state_dict(),
            "loss": test_results["test loss"],
        }
        checkpoint_file_name = f"{datetime.now().strftime('%Y%m%d%H%M%S')}.pth"
        checkpoint_path = os.path.join(checkpoints_folder, checkpoint_file_name)
        torch.save(checkpoint, checkpoint_path)
        _log_message(f"Saved checkpoint '{checkpoint_file_name}'")


def _enhance_image(input_file: str, output_file: str, device: torch.device) -> None:
    with torch.no_grad():
        input_image = torch_utils.load_image(input_file).unsqueeze(0).to(device)
        output_image = torch.clip(model(input_image).squeeze(0), 0.0, 255.0)
        torch_utils.save_image(output_image, output_file)


def _make_dirs_if_not_exists(path: str) -> None:
    if not os.path.exists(path):
        os.makedirs(path)


if __name__ == "__main__":
    """
    python py/main.py --train --limit 10000 --learning_rate=0.0001 --checkpoints_folder "models/checkpoints"
    python3 py/main.py --enhance -I "images/decompressed/val*.ppm" -O "images/enhanced" --checkpoints_folder "py/checkpoints"
    """
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )

    # Modes
    parser.add_argument("-t", "--train", action="store_true")
    parser.add_argument("-e", "--enhance", action="store_true")

    # Train mode parameters
    metrics = ["MSE", "YMSE", "DCT"]
    parser.add_argument("-n", "--number_of_epochs", type=int, default=-1)
    parser.add_argument("-lr", "--learning_rate", type=float, default=1e-4)
    parser.add_argument("-b", "--batch_size", type=int, default=16)
    parser.add_argument("-m", "--metric", choices=metrics, type=str, default=metrics[0])
    parser.add_argument("--test_size", type=float, default=0.07)

    # Enhance mode parameters
    parser.add_argument("-i", "--input_file", type=str)
    parser.add_argument("-o", "--output_file", type=str)

    parser.add_argument("-I", "--input_wildcard", type=str)
    parser.add_argument("-O", "--output_folder", type=str)

    # Common parameters
    parser.add_argument("-c", "--checkpoints_folder", type=str)
    parser.add_argument("-s", "--limit", type=int, default=10_000)

    args = parser.parse_args()

    device = torch_utils.get_device()
    if args.train:
        model, optimizer = _load_model(
            args.checkpoints_folder,
            device,
            create_optimizer=True,
            lr=args.learning_rate,
        )

        train_data_loader, test_data_loader = _create_data_loaders(
            inputs_folder="./images/decompressed",
            targets_folder="./images/decoded",
            limit=args.limit,
            test_size=args.test_size,
            batch_size=args.batch_size,
        )

        train_model(
            model=model,
            device=device,
            train_data_loader=train_data_loader,
            test_data_loader=test_data_loader,
            criterion=get_metric(args.metric),
            optimizer=optimizer,
            checkpoints_folder=args.checkpoints_folder,
            epochs_count=args.number_of_epochs,
        )
    elif args.enhance:
        model = _load_model(args.checkpoints_folder, device)
        if args.input_file is not None and args.output_file is not None:
            _make_dirs_if_not_exists(args.output_file)
            _enhance_image(args.input_file, args.output_file, device)
        elif args.input_wildcard is not None and args.output_folder is not None:
            _make_dirs_if_not_exists(args.output_folder)
            file_names = sorted([
                os.path.basename(file) for file in glob.glob(args.input_wildcard)
            ])
            if args.limit is None:
                args.limit = len(file_names)

            for file_name in tqdm(file_names[: args.limit]):
                _enhance_image(
                    input_file=os.path.join(os.path.dirname(args.input_wildcard), file_name),
                    output_file=os.path.join(args.output_folder, file_name),
                    device=device,
                )
        else:
            raise ValueError(
                "Expected --input_wildcard and --output_folder or --input_file and --output_file parameters"
            )
    else:
        raise ValueError("Supported only --train/--enhance modes")

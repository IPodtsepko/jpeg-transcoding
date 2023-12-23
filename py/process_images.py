import argparse
import os
import random
import numpy as np
import shutil
import filecmp
import matplotlib as mpl

from PIL import Image, JpegImagePlugin
from typing import Tuple, Generator
from joblib import Parallel, delayed
from matplotlib import pyplot as plt

plt.style.use("default")

mpl.rcParams['text.usetex'] = True
mpl.rcParams['font.family'] = 'serif'
mpl.rcParams['font.serif'] = 'cm'


def is_jpeg(file_name: str) -> bool:
    SUPPORTED_FORMATS = ".jpg", ".jpeg"
    return file_name.lower().endswith(SUPPORTED_FORMATS)


def call_jpegtran_for_image(input_file: str, output_folder: str):
    _, file_name = os.path.split(input_file)
    output_file = os.path.join(output_folder, file_name)

    command = f"jpegtran -arithmetic {input_file} > {output_file}"
    os.system(command)


def call_decoder_for_image(
    input_file: str,
    output_folder: str,
    mode: str | None,
    power: int,
    enhanced_folder: str | None,
):
    _, file_name = os.path.split(input_file)
    file_name, extension = os.path.splitext(file_name)

    if mode not in ("--compress", "--encode_residuals", "--decode_residuals"):
        extension = ".ppm"
    output_path = os.path.join(output_folder, file_name + extension)

    command = f"./build/Decoder -i {input_file} -o {output_path}"
    if mode is not None:
        command = f"{command} {mode} --power {power}"
    if mode in ("--encode_residuals", "--decode_residuals"):
        if enhanced_folder is None:
            return
        enhanced_file = os.path.join(enhanced_folder, file_name + ".ppm")
        if not os.path.exists(enhanced_file):
            return
        command = f"{command} -e {enhanced_file}"

    os.system(command)


def crop_image(input_file: str, output_folder: str, crop_size: Tuple[int, int]) -> None:
    _, file_name = os.path.split(input_file)
    file_name, extension = os.path.splitext(file_name)

    with Image.open(input_file) as im:
        width, height = im.size
        crop_width, crop_height = crop_size

        # Sampling:
        # 0: 4:4:4
        # 1: 4:2:2
        # 2: 4:2:0
        sampling = JpegImagePlugin.get_sampling(im)

        if sampling in (0, 2) and crop_width < width and crop_height < height:
            i = random.randint(0, width - crop_width)
            j = random.randint(0, height - crop_height)
            box = (i, j, i + crop_width, j + crop_height)
            output_path = os.path.join(output_folder, f"{file_name}_{i}_{j}{extension}")
            try:
                im.crop(box).save(output_path, qtables=im.quantization, subsampling=2)
            except ValueError as e:
                print(f"Cannot crop image {output_path} ('{e}'")
                raise e
            return output_path


def split_images(
    input_folder: str, validation_size: float = 0.2, test_size: float = 0.2
):
    image_files = list(list_of_jpegs(input_folder))

    total_count = len(image_files)
    validation_count = int(total_count * validation_size)
    test_count = int(total_count * test_size)

    def file_id(prefix, i):
        n = len(str(total_count))
        return f"{prefix}_{i:0{n}d}.jpeg"

    for i, file_name in enumerate(image_files):
        if i < validation_count:
            new_file_name = file_id("val", i)
        elif i < validation_count + test_count:
            new_file_name = file_id("tst", i - validation_count)
        else:
            new_file_name = file_id("trn", i - validation_count - test_count)
        new_file_name = os.path.join(input_folder, new_file_name)
        print(file_name, "->", new_file_name)
        shutil.move(file_name, new_file_name)


def list_of_jpegs(
    input_folder: str, limit: int | None = None
) -> Generator[str, None, None]:
    file_names = os.listdir(input_folder)
    if limit is not None:
        file_names = file_names[:limit]
    random.shuffle(file_names)

    for file_name in file_names:
        if not is_jpeg(file_name):
            continue
        yield os.path.join(input_folder, file_name)


def process_images_parallel(
    function,
    input_folder: str,
    n_jobs: int = 12,
    limit: int | None = None,
    *args,
    **kwargs,
):
    return Parallel(n_jobs=n_jobs)(
        delayed(function)(image, *args, **kwargs)
        for image in list_of_jpegs(input_folder, limit)
    )


def crop_images(
    input_folder: str,
    output_folder: str,
    crop_size: Tuple[int, int],
    limit: int | None = None,
) -> None:
    process_images_parallel(
        crop_image,
        input_folder,
        n_jobs=12,
        limit=limit,
        output_folder=output_folder,
        crop_size=crop_size,
    )


def call_decoder_for_images(
    input_folder: str,
    output_folder: str,
    mode: str | None,
    power: int | None,
    limit: int | None,
    enhanced_folder: str | None,
) -> None:
    process_images_parallel(
        call_decoder_for_image,
        input_folder,
        n_jobs=12,
        limit=limit,
        output_folder=output_folder,
        mode=mode,
        power=power,
        enhanced_folder=enhanced_folder,
    )


def call_jpegtran_for_images(
    input_folder: str,
    output_folder: str,
    limit: int | None,
) -> None:
    process_images_parallel(
        call_jpegtran_for_image,
        input_folder,
        n_jobs=12,
        limit=limit,
        output_folder=output_folder,
    )


def setdir(path) -> set[str]:
    return set(os.listdir(path))


def get_common_files(first_folder: str, second_folder: str) -> list[str]:
    return list(setdir(first_folder) & setdir(second_folder))


def calculate_statistics(input_folder: str, output_folder: str):
    def get_sizes(file):
        return [
            os.path.getsize(os.path.join(folder, file))
            for folder in (input_folder, output_folder)
        ]

    sizes_by_file = {file: get_sizes(file) for file in get_common_files(input_folder, output_folder)}

    total_input = sum(input_size for input_size, _ in sizes_by_file.values() )
    total_output = sum(output_size for _, output_size in sizes_by_file.values() )

    def calculate_compression_result(sizes):
        input_size, output_size = sizes
        return (input_size - output_size) / input_size * 100

    results = {
        file: calculate_compression_result(sizes)
        for file, sizes in sizes_by_file.items()
    }

    values = np.array(list(results.values()))

    # Statistics
    minimum = values.min()
    maximum = values.max()

    worst_result = None
    best_result = None
    for file, result in results.items():
        if result == minimum:
            worst_result = file
        if result == maximum:
            best_result = file

    values[values < 0] = 0

    average = values.mean()
    median = np.median(values)
    std = np.std(values)

    print("Compression statistics")
    print(f" - Minimum: {worst_result}")
    print(f" - Maximum: {best_result} ({maximum})")
    print(f" - Average: {average}")
    print(f" - Median: {median}")
    print(f" - Standard deviation: {std}")
    print(f" - Total: {(total_input - total_output) / total_input * 100}")

    plt.boxplot(values, 0, "")
    plt.savefig("boxplot.png", dpi=300)

    plt.clf()

    hist, bins = np.histogram(values, bins=20)
    bin_centers = (bins[1:] + bins[:-1]) / 2

    plt.plot(bin_centers, hist)
    plt.grid(axis='y')
    plt.tick_params(axis='both', length=0)

    ax = plt.gca()

    # Скрытие границ
    ax.spines['top'].set_visible(False)
    ax.spines['right'].set_visible(False)
    ax.spines['bottom'].set_visible(False)
    ax.spines['left'].set_visible(False)

    plt.xlabel("Compression, \%")
    plt.savefig('bitrade_distribution.png', dpi=300)

    shutil.copy(src=os.path.join(input_folder, worst_result), dst="worst.jpeg")
    shutil.copy(src=os.path.join(input_folder, best_result), dst="best.jpeg")


def test_trancoder(original_images_folder: str, transdecoded_images_folder: str):
    ok = True
    for file_name in get_common_files(
        original_images_folder, transdecoded_images_folder
    ):
        original_file_name, transdecoded_file_name = [
            os.path.join(folder, file_name)
            for folder in (original_images_folder, transdecoded_images_folder)
        ]
        if not filecmp.cmp(original_file_name, transdecoded_file_name):
            print(
                f"- files '{original_file_name}' and '{transdecoded_file_name}' differ"
            )
            ok = False
    print(f"Test result: {'succeeded' if ok else 'failed'}")


def main() -> int:
    """
    A utility for performing various image processing tasks required for the
    neural network image transcoding process.

    Usage examples:

    - To split a set of images into training, validation, and test sets:
    python3 py/process_images.py --split -i images/01-original-images --validation_size 0.1 --test_size 0.2

    - To crop images to specified dimensions:
    python3 py/process_images.py --crop -i "images/01-original-images" -o "images/02-croped" --height 320 --width 320

    - To decode JPEG images:
    python3 py/process_images.py --decode -i "images/02-croped" -o "images/03-decoded"

    - To prepare images with zeroed DCT coefficients:
    python3 py/process_images.py --zero_out_and_decode -i "images/02-croped" -o "images/04-decompressed" --power 16

    - To transcode images after processing them with QE-CNN:
    python3 py/process_images.py --encode_residuals -i "images/02-croped" -o "images/06-transcoded" -e "images/05-enhanced" --power 16

    - To replace Huffman coding with arithmetic coding using Jpegtran:
    python3 py/process_images.py --arithmetic -i "images/02-croped" -o "images/08-jpegtran-output"
    python3 py/process_images.py --arithmetic -i "images/06-transcoded" -o "images/09-jpegtran-output-for-transcoded"

    - To calculate compression ratio and statistics:
    python3 py/process_images.py --statistics -i "images/02-croped" -o "images/06-transcoded"
    python3 py/process_images.py --statistics -i "images/02-croped" -o "images/08-jpegtran-output"
    python3 py/process_images.py --statistics -i "images/02-croped" -o "images/09-jpegtran-output-for-transcoded"
    python3 py/process_images.py --statistics -i "images/02-croped" -o "images/10-lljpeg-output"
    python3 py/process_images.py --statistics -i "images/02-croped" -o "images/11-lljpeg-output-for-transcoded"

    - To transdecode:
    python3 py/process_images.py --decode_residuals -i "images/06-transcoded" -o "images/07-transdecoded" -e "images/05-enhanced" --power 16

    - To verify that the results of the transdecoding match the original images:
    python3 py/process_images.py --test_transcoder -i "images/02-croped" -o "images/07-transdecoded"
    """

    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        description="Image processing utils",
    )

    parser.add_argument("--input_folder", "-i", type=str, required=True)
    parser.add_argument("--output_folder", "-o", type=str)
    parser.add_argument("--enhanced_folder", "-e", type=str)

    modes = parser.add_mutually_exclusive_group(required=True)
    modes_alternatives = {
        "crop": "Crop images",
        "zero_out_and_decode": "Filter DCT coefficients and then decode images",
        "decode": "Decode JPEG images",
        "encode_residuals": "TODO",
        "decode_residuals": "TODO",
        "split": "TODO",
        "arithmetic": "TODO",
        "statistics": "TODO",
        "test_transcoder": "TODO",
    }
    for mode, description in modes_alternatives.items():
        modes.add_argument(f"--{mode}", help=description, action="store_true")

    parser.add_argument("--width", "-W", type=int, help="Width for cropping")
    parser.add_argument("--height", "-H", type=int, help="Height for cropping")
    parser.add_argument("--power", "-p", type=int)
    parser.add_argument("--limit", "-l", type=int)
    parser.add_argument("--validation_size", type=float, default=0.1)
    parser.add_argument("--test_size", type=float, default=0.2)

    args = parser.parse_args()

    if args.crop and (args.width is None or args.height is None):
        parser.error("--height and --width are required when --crop is specified")

    if (
        args.zero_out_and_decode or args.encode_residuals or args.decode_residuals
    ) and args.power is None:
        parser.error("--power is required when --zero_out_and_decode/--encode_residuals/--decode_residuals is specified")

    if not args.split and not os.path.exists(args.output_folder):
        os.makedirs(args.output_folder)

    if args.crop:
        crop_images(
            input_folder=args.input_folder,
            output_folder=args.output_folder,
            crop_size=(args.width, args.height),
            limit=args.limit,
        )
    elif args.split:
        split_images(
            input_folder=args.input_folder,
            validation_size=args.validation_size,
            test_size=args.test_size,
        )
    elif args.statistics:
        calculate_statistics(
            input_folder=args.input_folder,
            output_folder=args.output_folder,
        )
    elif args.test_transcoder:
        test_trancoder(
            original_images_folder=args.input_folder,
            transdecoded_images_folder=args.output_folder,
        )
    elif args.arithmetic:
        call_jpegtran_for_images(
            input_folder=args.input_folder,
            output_folder=args.output_folder,
            limit=args.limit,
        )
    else:
        call_decoder_for_images(
            input_folder=args.input_folder,
            output_folder=args.output_folder,
            mode={
                # args.compress: "--compress",
                args.zero_out_and_decode: "--compress-and-decode",
                # args.decompress: "--decompress",
                args.encode_residuals: "--encode_residuals",
                args.decode_residuals: "--decode_residuals",
                args.decode: None,
            }[True],
            power=args.power,
            limit=args.limit,
            enhanced_folder=args.enhanced_folder,
        )
        ####


if __name__ == "__main__":
    main()

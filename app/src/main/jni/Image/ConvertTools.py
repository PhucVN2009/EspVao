from PIL import Image
import os
import datetime
import array

def get_image_create_date(image_path):
    try:
        with Image.open(image_path) as img:
            info = img.info
            if "DateTimeOriginal" in info:
                create_date_str = info["DateTimeOriginal"]
                create_date = datetime.datetime.strptime(create_date_str, "%Y:%m:%d %H:%M:%S")
                return create_date.strftime("%Y-%m-%d %H:%M:%S")
    except Exception as e:
        print(f"Unable to retrieve image creation date: {e}")

    return datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

def convert_to_png(image_path):
    try:
        with Image.open(image_path) as img:
            if img.format != 'PNG':
                new_path = os.path.splitext(image_path)[0] + '.png'
                img.save(new_path, format='PNG')
                print(f"Image converted to PNG: {new_path}")
                return new_path
            else:
                return image_path
    except Exception as e:
        print(f"Unable to convert image to PNG: {e}")
        return None

def image_to_cpp_array(image_path):
    converted_path = convert_to_png(image_path)
    if converted_path is None:
        return

    try:
        with open(converted_path, 'rb') as img_file:
            image_data = array.array('B', img_file.read())
    except Exception as e:
        print(f"Unable to read image data: {e}")
        return

    image_name = os.path.splitext(os.path.basename(converted_path))[0]
    size = len(image_data)
    create_date = get_image_create_date(converted_path)

    cpp_header_content = f'''
/*
    Image Name: {image_name}
    Size: {size}
    Create Date: {create_date}
*/
const unsigned char {image_name}[] = {{
    {', '.join(f'0x{byte:02X}' for byte in image_data)}
}};
'''

    header_file_path = f'{image_name}.h'
    with open(header_file_path, 'w') as header_file:
        header_file.write(cpp_header_content)

    print(f"File {header_file_path} created successfully.")

if __name__ == "__main__":
    image_path = input("Enter the image path: ").strip()
    if os.path.exists(image_path):
        image_to_cpp_array(image_path)
    else:
        print("Invalid image path.")

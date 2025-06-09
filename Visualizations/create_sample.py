import wave
import struct
import os

def convert_wav_to_amplitude_text(wav_file_path: str, output_text_file_path: str):
    """
    Reads a .wav file in 16-bit chunks, extracts the amplitudes,
    and writes them to a .txt file.

    Args:
        wav_file_path (str): The path to the input .wav file.
        output_text_file_path (str): The path where the output .txt file
                                      containing amplitudes will be saved.
    """
    try:
        # Open the WAV file in read mode
        with wave.open(wav_file_path, 'rb') as wf:
            # Get audio file parameters
            n_channels = wf.getnchannels()      # Number of audio channels (e.g., 1 for mono, 2 for stereo)
            samp_width = wf.getsampwidth()      # Sample width in bytes (e.g., 2 bytes for 16-bit)
            framerate = wf.getframerate()       # Frame rate (samples per second)
            n_frames = wf.getnframes()          # Total number of frames in the file

            print(f"WAV File Info:")
            print(f"  Channels: {n_channels}")
            print(f"  Sample Width (bytes): {samp_width}")
            print(f"  Frame Rate: {framerate} Hz")
            print(f"  Total Frames: {n_frames}")

            # Ensure the file is 16-bit
            if samp_width != 2:
                print(f"Warning: This script is optimized for 16-bit WAV files. "
                      f"Detected {samp_width * 8}-bit. "
                      f"Proceeding, but results might not be as expected for non-16-bit.")

            # Open the output text file in write mode
            with open(output_text_file_path, 'w') as out_file:
                # Read all frames at once
                frames = wf.readframes(n_frames)

                # Define the format string for unpacking 16-bit signed integers
                # '<h' means little-endian signed short (2 bytes)
                # 'h' means native-endian signed short (2 bytes)
                # '>'h' means big-endian signed short (2 bytes)
                # Assuming standard WAV files are little-endian for sample data.
                # Adjust if your WAV files are big-endian.
                format_string = '<' + 'h' * (len(frames) // samp_width)

                # Unpack the binary data into a tuple of integers
                # Each integer represents a 16-bit amplitude value
                amplitudes = struct.unpack(format_string, frames)

                # Iterate through the unpacked amplitudes and write them to the text file
                for i, amp in enumerate(amplitudes):
                    # For stereo files, amplitudes will be interleaved (L, R, L, R...)
                    # This code simply writes all amplitudes in order of appearance.
                    out_file.write(f"{amp / 2**16}\n")
                    if (i + 1) % (framerate * n_channels * 5) == 0: # Print progress every 5 seconds of audio
                        print(f"Processed {i + 1} amplitudes...")

            print(f"Successfully extracted amplitudes to '{output_text_file_path}'")

    except FileNotFoundError:
        print(f"Error: WAV file not found at '{wav_file_path}'")
    except wave.Error as e:
        print(f"Error reading WAV file: {e}. It might be corrupted or not a valid WAV format.")
    except struct.error as e:
        print(f"Error unpacking binary data: {e}. The WAV file might have an unexpected format or be corrupted.")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")

# --- Example Usage ---
if __name__ == "__main__":
    # Create a dummy WAV file for demonstration if it doesn't exist
    dummy_wav_file = "best-shot.wav"
    if not os.path.exists(dummy_wav_file):
        print("FILE DOESN'T EXIST!!!")
    else:
        print(f"Using existing dummy WAV file: {dummy_wav_file}")

    output_file = "amplitudes.txt"
    convert_wav_to_amplitude_text(dummy_wav_file, output_file)

    print("\n--- Content of generated amplitudes.txt (first 10 lines) ---")
    if os.path.exists(output_file):
        with open(output_file, 'r') as f:
            for i, line in enumerate(f):
                if i < 10:
                    print(line.strip())
                else:
                    break
        print(f"(... full content in '{output_file}')")

# Start index: 3470
# End index: 4493
r"""Run."""
import arithmetic_coding as ac


def write_binary(enc, value, bin_num):
    bin_v = "{0:b}".format(value).zfill(bin_num)
    freqs = ac.SimpleFrequencyTable([1, 2, 3, 4])
    # freqs = ac.SimpleFrequencyTable([1, 1])
    for i in range(bin_num):
        enc.write(freqs, int(bin_v[i]))


# 编码
w_file = open("try.bin", "wb")
bit_out = ac.CountingBitOutputStream(bit_out=ac.BitOutputStream(w_file))
enc = ac.ArithmeticEncoder(bit_out)
for i in range(13):
    write_binary(enc, i * 10, 8)
    write_binary(enc, i * 10, 8)
for i in range(13):
    write_binary(enc, i % 2, 2)
    write_binary(enc, i % 2, 2)
enc.finish()
bit_out.close()
filesize = bit_out.num_bits / 8
print(filesize)

# 解码
bit_in = ac.BitInputStream(open("try.bin", "rb"))
dec = ac.ArithmeticDecoder(bit_in)
freqs = ac.SimpleFrequencyTable([1, 1, 1, 1, 1, 1, 1])
for i in range(50):
    print(i, dec.read(freqs))

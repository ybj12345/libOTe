#include <cryptoTools/Crypto/PRNG.h>

#include "libOTe/Tools/bitpolymul/byte_inline_func.h"

#include "libOTe/Tools/bitpolymul/bitpolymul.h"


#include "libOTe/Tools/bitpolymul/bpmDefines.h"
#include <cryptoTools/Common/CLP.h>
#include <cryptoTools/Common/Timer.h>
#include <cryptoTools/Common/TestCollection.h>



#define bm_func1 bitpolymul
#define bm_func2 bitpolymul_2_128


using namespace oc;
using namespace bpm;

void bitpolymul_test(const CLP& cmd)
{
    

    uint64_t TEST_RUN = cmd.getOr("t", 4);
    uint64_t len = (1ull << cmd.getOr("n", 12));

    bpm::aligned_vector<uint64_t>
        poly1(len),
        poly2(len),
        poly3(len),
        rPoly1(len * 2),
        rPoly2(len * 2);

    if (u64(poly1.data()) % 32)
        throw RTE_LOC;
    if (u64(poly2.data()) % 32)
        throw RTE_LOC;
    if (u64(poly3.data()) % 32)
        throw RTE_LOC;
    if (u64(rPoly1.data()) % 32)
        throw RTE_LOC;
    if (u64(rPoly2.data()) % 32)
        throw RTE_LOC;
    
    bool random = true;
    oc::PRNG prng(oc::ZeroBlock);

    if (random)
    {
        prng.get(poly1.data(), poly1.size());
        prng.get(poly2.data(), poly2.size());
    }
    else
    {
        poly1[0] = 2;
        poly2[0] = 4;
    }


    bm_func1(rPoly1.data(), poly2.data(), poly1.data(), len);
    bm_func1(rPoly2.data(), poly1.data(), poly2.data(), len);

    if (32 >= len && cmd.isSet("v")) {
        printf("poly1 :"); u64_dump(poly1); puts("");
        printf("poly2 :"); u64_dump(poly2); puts("");

        printf("rPoly1 :"); u64_dump(rPoly1); puts("");

        if (rPoly1 != rPoly2) {
            printf("consistency fail: \n");
            printf("rPoly2 :"); u64_fdump(stdout, rPoly2); puts("");
        }
    }


    bpm::FFTPoly fft1, fft2, fft3, fft_12, fft_13;
    auto vecAdd = [](std::vector<oc::u64> a, std::vector<oc::u64>& b)
    {
        for (oc::u64 i = 0; i < a.size(); ++i)
            a[i] ^= b[i];
        return a;
    };

    if (cmd.isSet("timer"))
    {
        oc::Timer timer;
        timer.setTimePoint("start");
        for (uint64_t i = 0; i < TEST_RUN; i++)
        {
            bm_func2(rPoly1.data(), poly2.data(), poly1.data(), len);
        }
        timer.setTimePoint("end");
        std::cout << "timer " << timer << std::endl;
    }

    uint64_t fail_count = 0;
    uint64_t chk = 0;
    for (uint64_t i = 0; i < TEST_RUN; i++)
    {
        //prng.get(poly1.data(), poly1.size());
        //prng.get(poly2.data(), poly2.size());
        for (uint64_t q = 0; q < len; q++) { poly1[q] = q + 1 + i; }   // i * i + 321434123377;
        for (uint64_t q = 0; q < len; q++) { poly2[q] = q + 2 + i; }   // i * i + 463254234534;
        for (uint64_t q = 0; q < len; q++) { poly3[q] = q + 3 + i; }   // i * i + 463254234534;



        auto back1 = poly1;
        auto back2 = poly2;

        bm_func1(rPoly1.data(), poly2.data(), poly1.data(), len);
        bm_func2(rPoly2.data(), poly2.data(), poly1.data(), len);


        if (rPoly1 != rPoly2) {
            fail_count++;
            if (cmd.isSet("v"))
            {

                printf("consistency fail: %d.\n", int(i));
                printf("res1:"); u64_fdump(stdout, rPoly1); puts("");
                printf("res2:"); u64_fdump(stdout, rPoly2); puts("");

                bitpolymul_2_64(rPoly1.data(), poly2.data(), poly1.data(), len);

                printf("res3:"); u64_fdump(stdout, rPoly1); puts("");

                if (back1 != poly1) printf("back 1 failed");
                if (back2 != poly2) printf("back 2 failed");
                printf("\n");
            }
        }


        fft1.encode(poly1);
        fft2.encode(poly2);
        fft3.encode(poly3);

        std::vector<oc::u64>
            poly_12(len * 2),
            poly_13(len * 2),
            fft_r(len * 2);

        fft_12.mult(fft1, fft2);
        fft_13.mult(fft1, fft3);

        fft_12.decode(poly_12, false);
        fft_13.decode(poly_13, false);

        bpm::FFTPoly fft_12_13;
        fft_12_13.add(fft_13, fft_12);

        auto poly_r = vecAdd(poly_12, poly_13);
        fft_12_13.decode(fft_r);

        if (poly_r != fft_r)
        {

            std::cout << "add plain failed." << std::endl;
            printf("poly_r:"); u64_fdump(stdout, poly_r); puts("");
            printf("fft_r: "); u64_fdump(stdout, fft_r); puts("");
            chk++;
        }

    }
    
    if (fail_count || chk)
        throw UnitTestFail(LOCATION);
}

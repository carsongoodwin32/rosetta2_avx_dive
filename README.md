# rosetta2 avx/2 implementation experimentation

## What is this repo about?
This repo contains the results of AVX and AVX2 code experiments I ran on June 11, 2024 on the macOS 15.0 Developer Beta version of Rosetta2

The files contained in this repo include:
- Code run on Rosetta that includes SSE2, AVX, and AVX2 codepaths
- Separate AVX and AVX2 only versions of that code
- Rosetta aot compilation files for that code
- Full Ghidra Disassemblies of those AOT files

## Preamble and history

When I heard that Apple had released Game Porting Toolkit 2, I was ecstatic. This was a sequel to something that I was really enthusiastic about when it first came out. 

Last year at WWDC when Game Porting Toolkit 1 came out, I was loving the fact that Apple was finally throwing their hat into the ring of translation layers for Windows games on macOS. For a long time we’ve had stuff like MoltenGL, MoltenVK, ANGLE, and others that made it sorta possible to translate older titles to run on mac’s, but with GPTK it was all about the shiny new x86_64 DX12 titles! It was exciting and it just worked! 

Well, it worked for a whole lot of things, but there’s always a couple hairs in the soup aren’t there?

You see, for YEARS *almost* every x86 processor has been built with something called AVX which is an instruction set addition that allows for easier manipulation of vectors and the like. You can read about it [here](https://en.wikipedia.org/wiki/Advanced_Vector_Extensions) if you want to know more, but for the purposes of this explanation, just know that AVX (and it’s younger sibling AVX2) are instructions that are included on just about any x86_64 processor worth its sand.

AVX, from the looks of it, is generally pretty useful for games. A lot of games that have been coming out recently have started using it. I mean, If the vast majority of the market is able to run these kinds of codepaths, and it allows you to do your work more efficiently, why wouldn’t you as a game developer take advantage of it?

Well this is where we kinda start running into issues with Game Porting Toolkit (henceforth known as GPTK in this post).

GPTK is built on a number of technologies on the CPU side, those being mainly Rosetta 2, our x86_64 -> ARM64 translation layer, and Wine. Wine is there to translate the Windows api side of things to something macOS can understand, while Rosetta 2 sits under that and transforms the x86_64 code into ARM64 code that the M Series processors can understand. All of this comes together pretty nicely until you start trying to run AVX code. 

AVX is blatantly NOT supported in macOS versions under 14.x running on M Series chipsets. Apple has a whole web page on how to circumvent this in the porting process, but generally they state: “Rosetta translates all x86_64 instructions, but it doesn’t support the execution of some newer instruction sets and processor features, such as AVX, AVX2, and AVX512 vector instructions”. What this means is sorta vague (what do you mean it translates all instructions, but doesn’t execute AVX specifically? Do you mean this code has been emitted this whole time, but hasn’t run due to licensing??), but just know that when you run AVX code without a check to see if AVX is supported, your app will crash under Rosetta*.

*At least until June 10th when macOS 15.0 Developer Beta was released. 

## Theory

Despite this being very clearly a Rosetta2 addition, the only place I see this being advertised is under the GPTK2 banner. You don’t even have to have GPTK2 downloaded or anywhere on your system to run AVX2 code, it just works-ish! Users on Twitter and Discord have been reporting that a lot of games that didn’t previously run, now run great! There are some caveats, but we’ll get to that a little later.

But hold up, how is Apple doing this? Well I don’t know exactly. But I want to!
Well, there are generally two paths that Apple can take to translate and run AVX2 code from my understanding:

path 1 (This is what most x86_64 Translation Layers will do going forward):
- Translate AVX and AVX2 to equivalent NEON instructions.

path 2 (This is specifically something only Apple can do as of right now):
- Use AMX Trickery.

Apple, being Apple, is not disclosing exactly how they’re doing any of their translation, but we have a way to figure out what we want to know using a pretty great blog post that I’ll link to [here](https://ffri.github.io/ProjectChampollion/part1/).
Shoutouts to Koh M. Nakagawa for their guide on how to analyze Rosetta2 translations!

I’m specifically interested in how Apple is handling all of the 256bit vector instructions so let’s go!

## AVX Code and Explanation

First, we should probably write some code that we know works and produces correct results on an x86 based machine. My idea is just to take a very large vector of random integers, sum all of them, and output their sum. Not very complex, but since the major difference between AVX and AVX2 is 128bit int support and 256bit int support this will be fine for our use cases.

The assembly is, generally speaking, very easy.
Here is our AVX code:
```
   int sum[4] = {0, 0, 0, 0};
    __asm__ __volatile__ (
        "vpxor %%xmm1, %%xmm1, %%xmm1\n\t"
        :
        :
        : "%xmm1"
    );
    for (int i = 0; i < VECTOR_SIZE; i += 4) {
        __asm__ __volatile__ (
            "vmovdqu (%0), %%xmm0\n\t"
            "vpaddd %%xmm0, %%xmm1, %%xmm1\n\t"
            :
            : "r" (&a[i])
            : "%xmm0", "%xmm1"
        );
    }
    __asm__ __volatile__ (
        "vmovdqu %%xmm1, %0\n\t"
        : "=m" (sum)
        :
        : "%xmm1"
    );
    return sum[0] + sum[1] + sum[2] + sum[3];
```

The only changes we’ll make is changing the xmm0 and xmm1 registers to ymm0 and ymm1 registers to access the whole 256 bits and expanding sum to 8 slots so we can receive those 8 ints back.

It doesn’t really matter how this code works, but for the sake of this post I’ll explain.
- We take in a vector of length n
- Initialize ymm1 (where our output will be stored) to 0
- Grab 8 ints (32bitsx8=256bits) out of the vector and load them into ymm0.
- Add ymm0 to ymm1 and store the result in ymm1
- Loop n/8 times doing this until we have finished grabbing all there is to grab from the vector
- output the result stored in register ymm1 to the variable sum.
- sum sum and return the summed sum of sum.

This code works the same for AVX, just using 4 ints instead of 8 for a total of 128bits of data and looping accordingly.

I’ve written a little SIMD benchmark tool using this premise with SSE2, AVX, and AVX2 instructions that you can find in the code folder of this repo if you want to play along. It’s the same general code structure for the SSE2 codepath as well. I included SSE2 because it’s well supported on Rosetta2. It just uses the same registers as the AVX code (xmm0 and xmm1) but uses movdqu and paddd instead of the v versions of those. Same thing.

We can compile this under Windows to test that my code is doing what it should by running
```
 g++ -msse2 -mavx -mavx2 avxbench.cpp -o benchmark
```

And we can run the .exe that it generated.
The results are not too surprising to be honest (Run on a Dell XPS 15 with a 13700h):
```
---------------------------Average of 10 Runs-------------------------------
SSE2 vs AVX: 2.88142% runtime difference
SSE2 vs AVX2: 30.0684% runtime difference
AVX vs AVX2: 28.068% runtime difference
```

Generally we see under Windows that AVX and SSE2 are within margin of error runtime differences. This is expected. What is pretty awesome though is a 30% performance uplift on the AVX2 code path.

Alright time for the big reveal! Let’s see how AVX2 runs on Apple Silicon!
We can compile this same exact code under macOS by running:
```
g++ -msse2 -mavx -mavx2 -arch x86_64 avxbench.cpp -o benchmark
```

Let’s run the resulting binary and check out how great performance is!
```
---------------------------Average of 10 Runs------------------------------- 
SSE2 vs AVX: -0.147036% runtime difference 
SSE2 vs AVX2: -12.8756% runtime difference
AVX vs AVX2: -12.7102% runtime difference
```
Ouch.
Wait a minute.
```
Run 10: 
SSE2 Int Sum Result: -773110574 Time: 0.411268 seconds 
AVX Int Sum Result: -773110574 Time: 0.411619 seconds 
AVX2 Int Sum Result: -1826000660 Time: 0.463102 seconds
```
It’s worse and it’s… wrong? These sums should all be the same. What is going on here?

#Looking at the dissassembly

Okay let’s go ahead and check out the resulting Rosetta2 translation of this program and see if we can figure out what the heck is going on.

We have to disable SIP to do this, so if you ARE playing along, do this in a VM.
We’ll separate out just the AVX and AVX2 code paths into their bare minimums that they’ll need to function, recompile them, and run them to get their translated outputs. (All relevant files are in the code folder)

The disassemblies are waaay too long to put in here, but rest assured they are in the code folder. First off:

- The AVX/2 code paths definitely use ARM NEON
We can see our vpxor, vmovdqu, vpaddd turned in to:
0133c  movi       v1.2D,#0x0
01384  mov       v0.16B,v24.16B
01390  add        v1.4S,v1.4S,v0.4S

And our 256b variants of those instructions turned in to:
01340 movi       v1.2D,#0x0
01344 movi       v24.2D,#0x0
013d0 mov        v0.16B,v24.16B
013e0 add        v1.4S,v1.4S,v0.4S
013e4 add        v25.4S,v25.4S,v24.4S

Plus a bunch of surrounding code that I won’t throw here.

About all I can sum from this is that the 256b vpadds and vpxors are coming through correctly. Something about the surrounding code isn’t preserving something into the final output though…

This is supported by the fact that if we modify the AVX2 codepath to do:
```
    for (int i = 0; i < VECTOR_SIZE; i += 4) {
        __asm__ __volatile__ (
            "vmovdqu (%0), %%ymm0\n\t"
            "vpaddd %%ymm0, %%ymm1, %%ymm1\n\t"
            :
            : "r" (&a[i])
            : "%ymm0", "%ymm1"
        );
    }

```

Instead of what it should actually do:
```
    for (int i = 0; i < VECTOR_SIZE; i += 8) {
        __asm__ __volatile__ (
            "vmovdqu (%0), %%ymm0\n\t"
            "vpaddd %%ymm0, %%ymm1, %%ymm1\n\t"
            :
            : "r" (&a[i])
            : "%ymm0", "%ymm1"
        );
    }

``` 
which is increment through the vector by 256 bits each loop. We get the “correct” results. Albeit now our code is 120% slower than the AVX code path. Of course, this is broken completely on x86_64 because this is not how AVX2 should actually work to my knowledge.

## Final Thoughts

Well, That’s about all I have time for today. I might expand more on this topic in the future and I’ll edit this if a solution comes up in a newer version of macOS. We’ll see if we can diff the .aot files to see what they fixed!


#include "LoadFile/DnaFastaLoader.h"

#include <cctype>
#include <limits>

namespace dna {

/** 
* HELPER:
* One linear scan over a raw chunk: drop `>` lines, strip whitespace /
* slashes, emit uppercase bases, record ATG offsets with the same per-chunk phase
* as `readFile.js` (`totalChunkLength + i`). 
*/
 void processChunkSinglePass(const char *data, size_t currentChunk, juce::MemoryOutputStream &totalFileContent)
 {                              
   bool lineStart = true;
   bool skippingHeaderLine = false;
 
   for (size_t i = 0; i < currentChunk; ++i) {
     auto currentChar = (unsigned char)data[i];
 
     if (currentChar == '\r' || currentChar == '\n') {
       lineStart = true;
       skippingHeaderLine = false;
       continue;
     } else if (skippingHeaderLine) {
       continue;
     }
 
     if (lineStart && currentChar == '>') {
       skippingHeaderLine = true;
       continue;
     }
 
     if (std::isspace((int)currentChar) != 0)
       continue;
 
     if (currentChar == '\\' || currentChar == '/')
       continue;
 
       
     // Uppercase
     currentChar = (unsigned char)std::toupper((int)currentChar);


     // RNA conversion
     auto uChar = (unsigned char)'U';
     auto tChar = (unsigned char)'T';
     if(currentChar == uChar) {
      currentChar = tChar;
     }
 
     // Add it to the acumulator
     totalFileContent.write(&currentChar, 1);
     lineStart = false;
   }
 }
 
 /**
  * HELPER:
  * When we read a chunk we want to finsh the reading the current line
  */
 void finishReadingCurrentLine(juce::InputStream &fileStream, juce::MemoryOutputStream &currentChunk) {
   if (fileStream.isExhausted())
     return;
 
   // Infinite loop until explicitly broken below (e.g., on end-of-stream or newline
   while (true) {
     const int byte = fileStream.readByte();
 
     // byte < 0 occurs when `readByte()` reaches end of stream (EOF).
     if (byte < 0)
       break;
 
     currentChunk.writeByte((char)byte);
 
     if (byte == '\n')
       break;
   }
 }


// truncateFileNameForDisplay
juce::String DnaFastaLoader::truncateFileNameForDisplay(const juce::String &name) {
  if (name.length() <= 20) { return name; }
  return name.substring(0, 20) + "...";
}

// Process file
FileReadResult DnaFastaLoader::processFile(const juce::File &file, std::atomic<bool> *cancelRequested) {
  constexpr int CHUNK_SIZE = 200000;
  constexpr std::int64_t MAX_CHARACTERS = 900000000;

  FileReadResult fileReadResult;

  // Open disk file
  juce::FileInputStream fileStream(file);

  // Early return on fail
  if (!fileStream.openedOk()) {
    fileReadResult.error = "Could not open file: " + file.getFullPathName();
    return fileReadResult;
  }

  // If file exceeds MAX_CHARACRERS limit - we limit how much we read.
  auto fileLength = fileStream.getTotalLength();
  if (fileLength >= MAX_CHARACTERS) {
    const auto middleOfTheFile = (fileLength - MAX_CHARACTERS) / 2;
    fileStream.setPosition(std::max<juce::int64>(0, middleOfTheFile));
  }

  juce::MemoryOutputStream totalFileContent;
  std::int64_t sanitizedTotalChars = 0;

  // 
  // LOOP START
  while (true) {
    // Early exit if UI actions abort the read (new file chosen, component destroyed).
    // Multithread code for checking if pointer bool value for cancelled was updated
    if (cancelRequested != nullptr && cancelRequested->load(std::memory_order_relaxed)) {
      fileReadResult.cancelled = true;
      return fileReadResult;
    }

    // Stop when we have enough sequence or the file reader hit EOF
    if (sanitizedTotalChars >= MAX_CHARACTERS || fileStream.isExhausted()) {
      break;
    }

    // Read a CHUNK
    juce::MemoryBlock rawFileMemoryBlock((size_t)CHUNK_SIZE);

    // nRead stores the actual number of bytes read from the file into the memory block during this iteration.
    // We need it to ensure that downstream processing only uses the valid, newly-read portion of the buffer,
    // as the last chunk may not completely fill the buffer if the file ends.
    const int bytesToRead = static_cast<int>(rawFileMemoryBlock.getSize());
    const int nRead = juce::jmax(0, fileStream.read(rawFileMemoryBlock.getData(), bytesToRead));


    // For storing how much we read in this pass through of the loop
    juce::MemoryOutputStream currentChunk;

    // This writes the contents of the memory block `rawFileMemoryBlock` (up to
    // `nRead` bytes, which is the number of bytes successfully read from the
    // fileStream) into the `currentChunk` output fileStream so that the chunk can
    // be further processed or extended (e.g., with trailing lines).
    currentChunk.write(rawFileMemoryBlock.getData(), static_cast<size_t>(nRead));

    // Tail through the next newline so lines are not split mid-string.
    if (!fileStream.isExhausted()) {
      finishReadingCurrentLine(fileStream, currentChunk);
    }

    // Remove all non DNA related characters and uppercase all letters.
    processChunkSinglePass(reinterpret_cast<const char *>(currentChunk.getData()), currentChunk.getDataSize(), totalFileContent);

    sanitizedTotalChars = (std::int64_t)totalFileContent.getDataSize();
  }
  // LOOP END
  // 

  // If cancel landed right after finishing the loop, skip building the giant `String` (~outlet payload).
  if (cancelRequested != nullptr && cancelRequested->load(std::memory_order_relaxed)) {
    fileReadResult.cancelled = true;
    return fileReadResult;
  }

  // We need a byte pointer so we can index efficiently by position, since totalFileContent.getData()
  // returns a void* and does not provide indexable access; for triplet/codon scan, we must cast it.
  const unsigned char* fileContentByteSequence = static_cast<const unsigned char*>(totalFileContent.getData());

  //  Finalise: one `String` wrapping the totalFileContent (JS `chunks.join("")` already upper per byte)
  const auto finalLengthOfReadChunks = totalFileContent.getDataSize();

  // Reserve capacity for 250000 entries to reduce reallocations, as many
  // genomes have tens of thousands of ATG codons. Without reserve, every time
  // the vector runs out of space it reallocates aImplicit conversion loses integer precision: 'size_t' (aka 'unsigned long') to 'int'nd copies existing indices;
  fileReadResult.startCodonMap.reserve(250000);

  // Loop over the totalFileContent in triplets
  for (size_t i = 0; i + 2 < finalLengthOfReadChunks; i += 3) {
    if (fileContentByteSequence[i] == 'A' && fileContentByteSequence[i + 1] == 'T' && fileContentByteSequence[i + 2] == 'G') {
      fileReadResult.startCodonMap.push_back(static_cast<std::int64_t>(i));
    }
  }

  // This code constructs a juce::String (assigned to fileReadResult.dnaSequence) from the processed DNA bytes 
  // in totalFileContent. It uses the entire content as a character array, but if the total length 
  // exceeds the maximum value for an int (which is the largest value accepted by createStringFromData),
  // it truncates to std::numeric_limits<int>::max(). This ensures safe conversion from raw memory to String.
  fileReadResult.dnaSequence = juce::String::createStringFromData(
      reinterpret_cast<const char *>(totalFileContent.getData()),
      finalLengthOfReadChunks > (size_t)std::numeric_limits<int>::max()
          ? std::numeric_limits<int>::max()
          : static_cast<int>(finalLengthOfReadChunks));
   

  return fileReadResult;
}
} // namespace dna

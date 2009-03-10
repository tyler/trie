require 'trie'

TRIE_PATH = 'spec/test-trie'

describe Trie do
  before :each do
    @trie = Trie.new(TRIE_PATH);
    @trie.add('rocket')
    @trie.add('rock')
    @trie.add('frederico')
  end
  
  after :each do
    @trie.close
    File.delete('spec/test-trie/trie.br')
    File.delete('spec/test-trie/trie.tl')
    File.delete('spec/test-trie/trie.sbm')
  end

  describe :path do
    it 'returns the correct path' do
      @trie.path.should == TRIE_PATH
    end
  end

  describe :has_key? do
    it 'returns true for words in the trie' do
      @trie.has_key?('rocket').should be_true
    end

    it 'returns nil for words that are not in the trie' do
      @trie.has_key?('not_in_the_trie').should be_nil
    end
  end

  describe :get do
    it 'returns -1 for words in the trie without a weight' do
      @trie.get('rocket').should == -1
    end

    it 'returns nil if the word is not in the trie' do
      @trie.get('not_in_the_trie').should be_nil
    end
  end

  describe :add do
    it 'adds a word to the trie' do
      @trie.add('forsooth').should == true
      @trie.get('forsooth').should == -1
    end

    it 'adds a word with a weight to the trie' do
      @trie.add('chicka',123).should == true
      @trie.get('chicka').should == 123
    end
  end

  describe :delete do
    it 'deletes a word from the trie' do
      @trie.delete('rocket').should == true
      @trie.has_key?('rocket').should be_nil
    end
  end

  describe :children do
    it 'returns all words beginning with a given prefix' do
      children = @trie.children('roc')
      children.size.should == 2
      children.should include('rock')
      children.should include('rocket')
    end

    it 'returns nil if prefix does not exist' do
      @trie.children('ajsodij').should be_nil
    end

    it 'includes the prefix if the prefix is a word' do
      children = @trie.children('rock')
      children.size.should == 2
      children.should include('rock')
      children.should include('rocket')
    end
  end

  describe :walk_to_terminal do
    it 'returns the first word found along a path' do
      @trie.add 'anderson'
      @trie.add 'andreas'
      @trie.add 'and'

      @trie.walk_to_terminal('anderson').should == 'and'
    end

    it 'returns the first word and value along a path' do
      @trie.add 'anderson'
      @trie.add 'andreas'
      @trie.add 'and', 15

      @trie.walk_to_terminal('anderson',true).should == ['and', 15]
    end
  end
end

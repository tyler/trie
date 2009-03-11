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

    it 'adds values greater than 16-bit allows' do
      @trie.add('chicka', 72_000).should == true
      @trie.get('chicka').should == 72_000
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

  describe :root do
    it 'returns a TrieNode' do
      @trie.root.should be_an_instance_of(TrieNode)
    end

    it 'returns a different TrieNode each time' do
      @trie.root.should_not == @trie.root
    end
  end

  describe :save do
    it 'saves the trie to disk such that another trie can be spawned which will read succesfully' do
      @trie.add('omgwtf',123)
      @trie.save

      trie2 = Trie.new(TRIE_PATH)
      trie2.get('omgwtf').should == 123
    end
  end
end

describe TrieNode do
  before :each do
    @trie = Trie.new(TRIE_PATH);
    @trie.add('rocket',1)
    @trie.add('rock',2)
    @trie.add('frederico',3)
    @node = @trie.root
  end
  
  after :each do
    @trie.close
    File.delete('spec/test-trie/trie.br')
    File.delete('spec/test-trie/trie.tl')
    File.delete('spec/test-trie/trie.sbm')
  end

  describe :state do
    it 'returns the most recent state character' do
      @node.walk!('r')
      @node.state.should == 'r'
      @node.walk!('o')
      @node.state.should == 'o'
    end

    it 'is nil when no walk has occurred' do
      @node.state.should == nil
    end
  end

  describe :full_state do
    it 'returns the current string' do
      @node.walk!('r').walk!('o').walk!('c')
      @node.full_state.should == 'roc'
    end

    it 'is a blank string when no walk has occurred' do
      @node.full_state.should == ''
    end
  end
  
  describe :walk! do
    it 'returns the updated object when the walk succeeds' do
      other = @node.walk!('r')
      other.should == @node
    end

    it 'returns nil when the walk fails' do
      @node.walk!('q').should be_nil
    end
  end

  describe :value do
    it 'returns nil when the node is not terminal' do
      @node.walk!('r')
      @node.value.should be_nil
    end

    it 'returns a value when the node is terminal' do
      @node.walk!('r').walk!('o').walk!('c').walk!('k')
      @node.value.should == 2
    end
  end

  describe :terminal? do
    it 'returns true when the node is a word end' do
      @node.walk!('r').walk!('o').walk!('c').walk!('k')
      @node.should be_terminal
    end

    it 'returns nil when the node is not a word end' do
      @node.walk!('r').walk!('o').walk!('c')
      @node.should_not be_terminal
    end
  end

  describe :leaf? do
    it 'returns true when this is the end of a branch of the trie' do
      @node.walk!('r').walk!('o').walk!('c').walk!('k').walk!('e').walk!('t')
      @node.should be_leaf
    end

    it 'returns nil when there are more splits on this branch' do
      @node.walk!('r').walk!('o').walk!('c').walk!('k')
      @node.should_not be_leaf
    end
  end

  describe :clone do
    it 'creates a new instance of this node which is not this node' do
      new_node = @node.clone
      new_node.should_not == @node
    end

    it 'matches the state of the current node' do
      new_node = @node.clone
      new_node.state.should == @node.state
    end

    it 'matches the full_state of the current node' do
      new_node = @node.clone
      new_node.full_state.should == @node.full_state
    end
  end
end
